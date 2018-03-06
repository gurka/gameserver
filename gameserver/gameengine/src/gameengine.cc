/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Simon Sandström
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "gameengine.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <utility>

#include "player.h"
#include "playerctrl.h"
#include "creature.h"
#include "creaturectrl.h"
#include "item.h"
#include "position.h"
#include "world.h"
#include "logger.h"
#include "tick.h"

namespace
{

struct RecursiveTask
{
  RecursiveTask(const std::function<void(const RecursiveTask&)>& f) : f(f) {}

  void operator()() const
  {
    f(*this);
  }

  std::function<void(const RecursiveTask&)> f;
};

}

GameEngine::GameEngine(TaskQueue* taskQueue,
                       std::string loginMessage,
                       World* world)
  : taskQueue_(taskQueue),
    loginMessage_(std::move(loginMessage)),
    world_(world)
{
}

void GameEngine::spawn(const std::string& name, PlayerCtrl* player_ctrl)
{
  taskQueue_->addTask(Creature::INVALID_ID, [this, name, player_ctrl]()
  {
    // Create the Player
    Player newPlayer{name};
    auto creatureId = newPlayer.getCreatureId();

    // Store the Player and the PlayerCtrl
    playerPlayerCtrl_.insert({creatureId, {std::move(newPlayer), player_ctrl}});

    // Get the Player again, since newPlayed is moved from
    auto& player = getPlayer(creatureId);

    LOG_DEBUG("%s: Spawn player: %s", __func__, player.getName().c_str());

    // Tell PlayerCtrl its CreatureId
    player_ctrl->setPlayerId(player.getCreatureId());

    // Spawn the player
    auto rc = world_->addCreature(&player, player_ctrl, Position(222, 222, 7));
    if (rc != World::ReturnCode::OK)
    {
      LOG_ERROR("%s: Could not spawn player", __func__);
      // TODO(gurka): Maybe let Protocol know that the player couldn't spawn, instead of time out?
    }
    else
    {
      player_ctrl->sendTextMessage(0x11, loginMessage_);
    }
  });
}

void GameEngine::despawn(CreatureId creatureId)
{
  taskQueue_->addTask(creatureId, [this, creatureId]()
  {
    LOG_DEBUG("%s: Despawn player, creature id: %d", __func__, creatureId);
    world_->removeCreature(creatureId);

    // Remove Player and PlayerCtrl
    playerPlayerCtrl_.erase(creatureId);

    // Remove any queued tasks for this player
    taskQueue_->cancelAllTasks(creatureId);
  });
}

void GameEngine::move(CreatureId creatureId, Direction direction)
{
  auto task = RecursiveTask([this, creatureId, direction](const RecursiveTask& task)
  {
    LOG_DEBUG("%s: creature id: %d", __func__, creatureId);

    auto* player_ctrl = getPlayerCtrl(creatureId);

    auto rc = world_->creatureMove(creatureId, direction);
    if (rc == World::ReturnCode::MAY_NOT_MOVE_YET)
    {
      LOG_DEBUG("%s: player move delayed, creature id: %d", __func__, creatureId);
      const auto& creature = world_->getCreature(creatureId);
      taskQueue_->addTask(creatureId, creature.getNextWalkTick() - Tick::now(), task);
    }
    else if (rc == World::ReturnCode::THERE_IS_NO_ROOM)
    {
      player_ctrl->sendCancel("There is no room.");
    }
  });

  taskQueue_->addTask(creatureId, task);
}

void GameEngine::movePath(CreatureId creatureId, const std::deque<Direction>& path)
{
  auto& player = getPlayer(creatureId);
  player.queueMoves(path);

  auto task = RecursiveTask([this, creatureId](const RecursiveTask& task)
  {
    auto& player = getPlayer(creatureId);

    // Make sure that the queued moves hasn't been canceled
    if (player.hasQueuedMove())
    {
      auto rc = world_->creatureMove(creatureId, player.getNextQueuedMove());

      if (rc == World::ReturnCode::OK)
      {
        // Player moved, pop the move from the queue
        player.popNextQueuedMove();
      }
      else if (rc != World::ReturnCode::MAY_NOT_MOVE_YET)
      {
        // If we neither got OK nor MAY_NOT_MOVE_YET: stop here and cancel all queued moves
        cancelMove(creatureId);
      }

      if (player.hasQueuedMove())
      {
        // If there are more queued moves, e.g. we moved but there are more moves or we were not allowed
        // to move yet, add a new task
        taskQueue_->addTask(creatureId, player.getNextWalkTick() - Tick::now(), task);
      }
    }
  });

  taskQueue_->addTask(creatureId, task);
}

void GameEngine::cancelMove(CreatureId creatureId)
{
  LOG_DEBUG("%s: creature id: %d", __func__, creatureId);

  auto& player = getPlayer(creatureId);
  auto* player_ctrl = getPlayerCtrl(creatureId);
  if (player.hasQueuedMove())
  {
    player.clearQueuedMoves();
    player_ctrl->cancelMove();
  }

  // Don't cancel the task, just let it expire and do nothing
}

void GameEngine::turn(CreatureId creatureId, Direction direction)
{
  taskQueue_->addTask(creatureId, [this, creatureId, direction]()
  {
    LOG_DEBUG("%s: Player turn, creature id: %d", __func__, creatureId);
    world_->creatureTurn(creatureId, direction);
  });
}

void GameEngine::say(CreatureId creatureId, uint8_t type, const std::string& message, const std::string& receiver, uint16_t channelId)
{
  taskQueue_->addTask(creatureId, [this, creatureId, type, message, receiver, channelId]()
  {
    LOG_DEBUG("%s: creatureId: %d, message: %s", __func__, creatureId, message.c_str());

    // Check if message is a command
    if (message.size() > 0 && message[0] == '/')
    {
      // Extract everything after '/'
      auto fullCommand = message.substr(1, std::string::npos);

      // Check if there is arguments
      auto space = fullCommand.find_first_of(' ');
      std::string command;
      std::string option;
      if (space == std::string::npos)
      {
        command = fullCommand;
        option = "";
      }
      else
      {
        command = fullCommand.substr(0, space);
        option = fullCommand.substr(space + 1);
      }

      // Check commands
      if (command == "debug" || command == "debugf")
      {
        // Different position for debug / debugf
        Position position;

        // Show debug info for a tile
        if (command == "debug")
        {
          // Show debug information on player tile
          position = world_->getCreaturePosition(creatureId);
        }
        else if (command == "debugf")
        {
          // Show debug information on tile in front of player
          const auto& player = getPlayer(creatureId);
          position = world_->getCreaturePosition(creatureId).addDirection(player.getDirection());
        }

        const auto& tile = world_->getTile(position);

        std::ostringstream oss;
        oss << "Position: " << position.toString() << "\n";

        for (const auto& item : tile.getItems())
        {
          oss << "Item: " << item.getItemId() << " (" << item.getName() << ")\n";
        }

        for (const auto& creatureId : tile.getCreatureIds())
        {
          oss << "Creature: " << creatureId << "\n";
        }

        getPlayerCtrl(creatureId)->sendTextMessage(0x13, oss.str());
      }
      else if (command == "put")
      {
        std::istringstream iss(option);
        int itemId = 0;
        iss >> itemId;

        if (itemId < 100 || itemId > 2381)
        {
          getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Invalid itemId");
        }
        else
        {
          const auto& player = getPlayer(creatureId);
          auto position = world_->getCreaturePosition(creatureId).addDirection(player.getDirection());
          world_->addItem(itemId, position);
        }
      }
      else
      {
        getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Invalid command");
      }
    }
    else
    {
      world_->creatureSay(creatureId, message);
    }
  });
}

void GameEngine::moveItemFromPosToPos(CreatureId creatureId, const Position& fromPosition, int fromStackPos, int itemId, int count, const Position& toPosition)
{
  if (itemId == 99)
  {
    // TODO(gurka): Figure out how to handle this (move Creature), it's not trivial
    return;
  }

  taskQueue_->addTask(creatureId, [this, creatureId, fromPosition, fromStackPos, itemId, count, toPosition]()
  {
    LOG_DEBUG("%s: Move Item from Tile to Tile, creature id: %d, from: %s, stackPos: %d, itemId: %d, count: %d, to: %s",
              __func__,
              creatureId,
              fromPosition.toString().c_str(),
              fromStackPos,
              itemId,
              count,
              toPosition.toString().c_str());

    World::ReturnCode rc = world_->moveItem(creatureId, fromPosition, fromStackPos, itemId, count, toPosition);

    switch (rc)
    {
      case World::ReturnCode::OK:
      {
        break;
      }

      case World::ReturnCode::CANNOT_MOVE_THAT_OBJECT:
      {
        getPlayerCtrl(creatureId)->sendCancel("You cannot move that object.");
        break;
      }

      case World::ReturnCode::CANNOT_REACH_THAT_OBJECT:
      {
        getPlayerCtrl(creatureId)->sendCancel("You are too far away.");
        break;
      }

      case World::ReturnCode::THERE_IS_NO_ROOM:
      {
        getPlayerCtrl(creatureId)->sendCancel("There is no room.");
        break;
      }

      default:
      {
        // TODO(gurka): Disconnect player?
        break;
      }
    }
  });
}

void GameEngine::moveItemFromPosToInv(CreatureId creatureId, const Position& fromPosition, int fromStackPos, int itemId, int count, int toInventoryId)
{
  taskQueue_->addTask(creatureId, [this, creatureId, fromPosition, fromStackPos, itemId, count, toInventoryId]()
  {
    LOG_DEBUG("%s: Move Item from Tile to Inventory, creature id: %d, from: %s, stackPos: %d, itemId: %d, count: %d, toInventoryId: %d",
              __func__,
              creatureId,
              fromPosition.toString().c_str(),
              fromStackPos,
              itemId,
              count,
              toInventoryId);

    auto& player = getPlayer(creatureId);
    auto* player_ctrl = getPlayerCtrl(creatureId);

    // Check if the player can reach the fromPosition
    if (!world_->creatureCanReach(creatureId, fromPosition))
    {
      player_ctrl->sendCancel("You are too far away.");
      return;
    }

    // Get the Item from the position
    auto item = world_->getTile(fromPosition).getItem(fromStackPos);
    if (!item.isValid() || item.getItemId() != itemId)
    {
      LOG_ERROR("%s: Could not find Item with given itemId at fromPosition", __func__);
      return;
    }

    // Check if we can add the Item to that inventory slot
    auto& equipment = player.getEquipment();
    if (!equipment.canAddItem(item, toInventoryId))
    {
      player_ctrl->sendCancel("You cannot equip that object.");
      return;
    }

    // Remove the Item from the fromTile
    World::ReturnCode rc = world_->removeItem(itemId, count, fromPosition, fromStackPos);
    if (rc != World::ReturnCode::OK)
    {
      LOG_ERROR("%s: Could not remove item %d (count %d) from %s (stackpos: %d)",
                __func__,
                itemId,
                count,
                fromPosition.toString().c_str(),
                fromStackPos);
      // TODO(gurka): Disconnect player?
      return;
    }

    // Add the Item to the inventory
    equipment.addItem(item, toInventoryId);
    player_ctrl->onEquipmentUpdated(player, toInventoryId);
  });
}

void GameEngine::moveItemFromInvToPos(CreatureId creatureId, int fromInventoryId, int itemId, int count, const Position& toPosition)
{
  taskQueue_->addTask(creatureId, [this, creatureId, fromInventoryId, itemId, count, toPosition]()
  {
    LOG_DEBUG("%s: Move Item from Inventory to Tile, creature id: %d, from: %d, itemId: %d, count: %d, to: %s",
              __func__,
              creatureId,
              fromInventoryId,
              itemId,
              count,
              toPosition.toString().c_str());

    auto& player = getPlayer(creatureId);
    auto* player_ctrl = getPlayerCtrl(creatureId);
    auto& equipment = player.getEquipment();

    // Check if there is an Item with correct itemId at the fromInventoryId
    auto item = equipment.getItem(fromInventoryId);
    if (!item.isValid() || item.getItemId() != itemId)
    {
      LOG_ERROR("%s: Could not find Item with given itemId at fromInventoryId", __func__);
      return;
    }

    // Check if the player can throw the Item to the toPosition
    if (!world_->creatureCanThrowTo(creatureId, toPosition))
    {
      player_ctrl->sendCancel("There is no room.");
      return;
    }

    // Remove the Item from the inventory slot
    if (!equipment.removeItem(item, fromInventoryId))
    {
      LOG_ERROR("%s: Could not remove item %d from inventory slot %d", __func__, itemId, fromInventoryId);
      return;
    }

    player_ctrl->onEquipmentUpdated(player, fromInventoryId);

    // Add the Item to the toPosition
    world_->addItem(item, toPosition);
  });
}

void GameEngine::moveItemFromInvToInv(CreatureId creatureId, int fromInventoryId, int itemId, int count, int toInventoryId)
{
  taskQueue_->addTask(creatureId, [this, creatureId, fromInventoryId, itemId, count, toInventoryId]()
  {
    LOG_DEBUG("%s: Move Item from Inventory to Inventory, creature id: %d, from: %d, itemId: %d, count: %d, to: %d",
              __func__,
              creatureId,
              fromInventoryId,
              itemId,
              count,
              toInventoryId);

    auto& player = getPlayer(creatureId);
    auto* player_ctrl = getPlayerCtrl(creatureId);
    auto& equipment = player.getEquipment();

    // TODO(gurka): Count

    // Check if there is an Item with correct itemId at the fromInventoryId
    auto item = equipment.getItem(fromInventoryId);
    if (!item.isValid() || item.getItemId() != itemId)
    {
      LOG_ERROR("%s: Could not find Item with given itemId at fromInventoryId", __func__);
      return;
    }

    // Check if we can add the Item to the toInventoryId
    if (!equipment.canAddItem(item, toInventoryId))
    {
      player_ctrl->sendCancel("You cannot equip that object.");
      return;
    }

    // Remove the Item from the fromInventoryId
    if (!equipment.removeItem(item, fromInventoryId))
    {
      LOG_ERROR("%s: Could not remove item %d from inventory slot %d",
                __func__,
                itemId,
                fromInventoryId);
      return;
    }

    // Add the Item to the to-inventory slot
    equipment.addItem(item, toInventoryId);

    player_ctrl->onEquipmentUpdated(player, fromInventoryId);
    player_ctrl->onEquipmentUpdated(player, toInventoryId);
  });
}

void GameEngine::useInvItem(CreatureId creatureId, int itemId, int inventoryIndex)
{
  LOG_DEBUG("%s: Use Item in inventory, creature id: %d, itemId: %d, inventoryIndex: %d",
            __func__,
            creatureId,
            itemId,
            inventoryIndex);

  // TODO(gurka): Fix
  //  world_->useItem(creatureId, itemId, inventoryIndex);
}

void GameEngine::usePosItem(CreatureId creatureId, int itemId, const Position& position, int stackPos)
{
  LOG_DEBUG("%s: Use Item at position, creature id: %d, itemId: %d, position: %s, stackPos: %d",
            __func__,
            creatureId,
            itemId,
            position.toString().c_str(),
            stackPos);

  // TODO(gurka): Fix
  //  world_->useItem(creatureId, itemId, position, stackPos);
}

void GameEngine::lookAtInvItem(CreatureId creatureId, int inventoryIndex, ItemId itemId)
{
  taskQueue_->addTask(creatureId, [this, creatureId, inventoryIndex, itemId]()
  {
    auto& player = getPlayer(creatureId);
    const auto& playerEquipment = player.getEquipment();

    if (!playerEquipment.hasItem(inventoryIndex))
    {
      LOG_DEBUG("%s: There is no item in inventoryIndex %u",
                __func__,
                inventoryIndex);
      return;
    }

    const auto& item = playerEquipment.getItem(inventoryIndex);

    if (item.getItemId() != itemId)
    {
      LOG_DEBUG("%s: Item at given inventoryIndex does not match given itemId, given itemId: %u inventory itemId: %u",
                __func__,
                itemId,
                item.getItemId());
      return;
    }

    if (!item.isValid())
    {
      LOG_DEBUG("%s: Item at given inventoryIndex is not valid", __func__);
      return;
    }

    std::ostringstream ss;

    if (!item.getName().empty())
    {
      if (item.isStackable() && item.getCount() > 1)
      {
        ss << "You see " << item.getCount() << " " << item.getName() << "s.";
      }
      else
      {
        ss << "You see a " << item.getName() << ".";
      }
    }
    else
    {
      ss << "You see an item with id " << itemId << ".";
    }

    if (item.hasAttribute("weight"))
    {
      ss << "\nIt weights " << item.getAttribute<float>("weight")<< " oz.";
    }

    if (item.hasAttribute("description"))
    {
      ss << "\n" << item.getAttribute<std::string>("description");
    }

    getPlayerCtrl(creatureId)->sendTextMessage(0x13, ss.str());
  });
}

void GameEngine::lookAtPosItem(CreatureId creatureId, const Position& position, ItemId itemId, int stackPos)
{
  taskQueue_->addTask(creatureId, [this, creatureId, position, itemId, stackPos]()
  {
    std::ostringstream ss;

    const auto& tile = world_->getTile(position);

    if (itemId == 99)
    {
      const auto& creatureIds = tile.getCreatureIds();
      if (!creatureIds.empty())
      {
        const auto& creatureId = creatureIds.front();
        const auto& creature = world_->getCreature(creatureId);
        ss << "You see " << creature.getName() << ".";
      }
      else
      {
        LOG_DEBUG("%s: No Creatures at given position: %s", __func__, position.toString().c_str());
        return;
      }
    }
    else
    {
      Item item;

      for (const auto& tileItem : tile.getItems())
      {
        if (tileItem.getItemId() == itemId)
        {
          item = tileItem;
          break;
        }
      }

      if (!item.isValid())
      {
        LOG_DEBUG("%s: No Item with itemId %d at given position: %s", __func__, itemId, position.toString().c_str());
        return;
      }

      if (!item.getName().empty())
      {
        if (item.isStackable() && item.getCount() > 1)
        {
          ss << "You see " << item.getCount() << " " << item.getName() << "s.";
        }
        else
        {
          ss << "You see a " << item.getName() << ".";
        }
      }
      else
      {
        ss << "You see an item with id " << itemId << ".";
      }

      // TODO(gurka): Can only see weight if standing next to the item
      if (item.hasAttribute("weight"))
      {
        ss << "\nIt weights " << item.getAttribute<float>("weight")<< " oz.";
      }

      if (item.hasAttribute("description"))
      {
        ss << "\n" << item.getAttribute<std::string>("description");
      }
    }

    getPlayerCtrl(creatureId)->sendTextMessage(0x13, ss.str());
  });
}
