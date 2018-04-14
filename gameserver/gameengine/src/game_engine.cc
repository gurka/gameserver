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

#include "game_engine.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <utility>

#include "player.h"
#include "player_ctrl.h"
#include "creature.h"
#include "creature_ctrl.h"
#include "item.h"
#include "position.h"
#include "world.h"
#include "logger.h"
#include "tick.h"

namespace
{

struct RecursiveTask
{
  RecursiveTask(const std::function<void(const RecursiveTask&, GameEngine* gameEngine)>& func)
    : func_(func)
  {
  }

  void operator()(GameEngine* gameEngine) const
  {
    func_(*this, gameEngine);
  }

  std::function<void(const RecursiveTask&, GameEngine* gameEngine)> func_;
};

}

GameEngine::GameEngine(GameEngineQueue* gameEngineQueue, World* world, std::string loginMessage)
  : gameEngineQueue_(gameEngineQueue),
    world_(world),
    loginMessage_(std::move(loginMessage)),
    containerManager_()
{
}

void GameEngine::spawn(const std::string& name, PlayerCtrl* player_ctrl)
{
  // Create the Player
  Player newPlayer{name};
  auto creatureId = newPlayer.getCreatureId();

  // Store the Player and the PlayerCtrl
  playerPlayerCtrl_.emplace(creatureId, PlayerPlayerCtrl{std::move(newPlayer), player_ctrl});

  // Get the Player again, since newPlayed is moved from
  auto& player = getPlayer(creatureId);

  LOG_DEBUG("%s: Spawn player: %s", __func__, player.getName().c_str());

  // Tell PlayerCtrl its CreatureId
  player_ctrl->setPlayerId(player.getCreatureId());

  // Inform ContainerManager of the new player
  containerManager_.playerSpawn(player_ctrl);

  // Spawn the player
  auto rc = world_->addCreature(&player, player_ctrl, Position(222, 222, 7));
  if (rc != World::ReturnCode::OK)
  {
    LOG_ERROR("%s: Could not spawn player", __func__);
    // TODO(simon): Maybe let Protocol know that the player couldn't spawn, instead of time out?
    // playerPlayerCtrl_.erase(creatureId);
    // player_ctrl->disconnect();
  }
  else
  {
    player_ctrl->sendTextMessage(0x11, loginMessage_);
  }
}

void GameEngine::despawn(CreatureId creatureId)
{
  LOG_DEBUG("%s: Despawn player, creature id: %d", __func__, creatureId);

  // Inform ContainerManager
  containerManager_.playerDespawn(getPlayerCtrl(creatureId));

  // Remove any queued tasks for this player
  gameEngineQueue_->cancelAllTasks(creatureId);

  // Finally despawn the player from the World
  // Note: this will free the PlayerCtrl, but requires Player to still be allocated
  world_->removeCreature(creatureId);

  // Remove Player and PlayerCtrl
  playerPlayerCtrl_.erase(creatureId);
}

void GameEngine::move(CreatureId creatureId, Direction direction)
{
  LOG_DEBUG("%s: creature id: %d", __func__, creatureId);

  auto* player_ctrl = getPlayerCtrl(creatureId);

  auto rc = world_->creatureMove(creatureId, direction);
  if (rc == World::ReturnCode::MAY_NOT_MOVE_YET)
  {
    LOG_DEBUG("%s: player move delayed, creature id: %d", __func__, creatureId);
    const auto& creature = world_->getCreature(creatureId);
    gameEngineQueue_->addTask(creatureId,
                              creature.getNextWalkTick() - Tick::now(),
                              [this, creatureId, direction](GameEngine* gameEngine)
    {
      move(creatureId, direction);
    });
  }
  else if (rc == World::ReturnCode::THERE_IS_NO_ROOM)
  {
    player_ctrl->sendCancel("There is no room.");
  }
}

void GameEngine::movePath(CreatureId creatureId, std::deque<Direction>&& path)
{
  getPlayer(creatureId).queueMoves(std::move(path));

  const auto task = RecursiveTask([this, creatureId](const RecursiveTask& task, GameEngine* gameEngine)
  {
    auto& player = getPlayer(creatureId);

    // Make sure that the queued moves hasn't been canceled
    if (player.hasQueuedMove())
    {
      const auto rc = world_->creatureMove(creatureId, player.getNextQueuedMove());

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
        gameEngineQueue_->addTask(creatureId, player.getNextWalkTick() - Tick::now(), task);
      }
    }
  });

  task(this);
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
  LOG_DEBUG("%s: Player turn, creature id: %d", __func__, creatureId);
  world_->creatureTurn(creatureId, direction);
}

void GameEngine::say(CreatureId creatureId, uint8_t type, const std::string& message, const std::string& receiver, uint16_t channelId)
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
}

void GameEngine::moveItem(CreatureId creatureId, const ItemPosition& fromPosition, const GamePosition& toPosition, int count)
{
  LOG_DEBUG("%s: creatureId: %d fromPosition: %s toPosition: %s count: %d",
            __func__,
            creatureId,
            fromPosition.toString().c_str(),
            toPosition.toString().c_str(),
            count);

  if (fromPosition.getItemId() == 0x63)
  {
    // Move Creature
    getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Not yet implemented.");
    return;
  }

  // Verify that the Item is at fromPosition and can be moved
  if (!getItem(creatureId, fromPosition))
  {
    LOG_ERROR("%s: could not find Item at fromPosition: %s", __func__, fromPosition.toString().c_str());
    return;
  }
  const auto item = *getItem(creatureId, fromPosition);

  // TODO(simon): verify that Item is movable

  // TODO(simon): check if toPosition points to a container item, and in that case change toPosition
  //              to point inside that container (if applicable)

  if (item.isContainer())
  {
    // TODO(simon): move of Container requires ContainerManager to recalculate and
    //              modify parentContainerId and rootItemPosition
    getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Not yet implemented.");
    return;
  }

  // Verify that the Item can be added to toPosition
  if (!canAddItem(creatureId, toPosition, item, count))
  {
    // TODO(simon): proper error message to player
    LOG_ERROR("%s: cannot add Item to toPosition: %s", __func__, toPosition.toString().c_str());
    return;
  }

  // Remove Item from fromPosition
  removeItem(creatureId, fromPosition, count);

  // Add Item to toPosition
  addItem(creatureId, toPosition, item, count);
}

void GameEngine::useItem(CreatureId creatureId, const ItemPosition& position, int newContainerId)
{
  LOG_DEBUG("%s: creatureId: %d position: %s newContainerId: %d",
            __func__,
            creatureId,
            position.toString().c_str(),
            newContainerId);

  auto* item = getItem(creatureId, position);
  if (!item)
  {
    LOG_ERROR("%s: could not find Item", __func__);
    return;
  }

  // TODO(simon): verify that player is close enough, if item is in world

  if (item->isContainer())
  {
    // Create a new Container if this is the first time that this item is used
    if (item->getContainerId() == Container::INVALID_ID)
    {
      item->setContainerId(containerManager_.createContainer(getPlayerCtrl(creatureId),
                                                             item->getItemId(),
                                                             position));
    }

    containerManager_.useContainer(getPlayerCtrl(creatureId), *item, newContainerId);
  }
}

void GameEngine::lookAt(CreatureId creatureId, const ItemPosition& position)
{
  LOG_DEBUG("%s: creatureId: %d position: %s",
            __func__,
            creatureId,
            position.toString().c_str());

  auto* item = getItem(creatureId, position);
  if (!item)
  {
    LOG_ERROR("%s: could not find Item", __func__);
    return;
  }

  // TODO(simon): verify that player is close enough, if item is in world

  if (item->isValid())
  {
    std::ostringstream ss;

    if (!item->getName().empty())
    {
      if (item->isStackable() && item->getCount() > 1)
      {
        ss << "You see " << item->getCount() << " " << item->getName() << "s.";
      }
      else
      {
        ss << "You see a " << item->getName() << ".";
      }
    }
    else
    {
      ss << "You see an item with id " << item->getItemId() << ".";
    }

    // TODO(simon): only if standing next to the item
    if (item->hasAttribute("weight"))
    {
      ss << "\nIt weights " << item->getAttribute<float>("weight")<< " oz.";
    }

    if (item->hasAttribute("description"))
    {
      ss << "\n" << item->getAttribute<std::string>("description");
    }

    getPlayerCtrl(creatureId)->sendTextMessage(0x13, ss.str());
  }
  else
  {
    LOG_ERROR("%s: could not find an Item at position: %s", __func__, position.toString().c_str());
  }
}

void GameEngine::closeContainer(CreatureId creatureId, int clientContainerId)
{
  LOG_DEBUG("%s: creatureId: %d clientContainerId: %d", __func__, creatureId, clientContainerId);
  containerManager_.closeContainer(getPlayerCtrl(creatureId), clientContainerId);
}

void GameEngine::openParentContainer(CreatureId creatureId, int clientContainerId)
{
  LOG_DEBUG("%s: creatureId: %d clientContainerId: %d", __func__, creatureId, clientContainerId);
  containerManager_.openParentContainer(getPlayerCtrl(creatureId), clientContainerId);
}

Item* GameEngine::getItem(CreatureId creatureId, const ItemPosition& position)
{
  // TODO(simon): verify ItemId

  const auto& gamePosition = position.getGamePosition();
  if (gamePosition.isPosition())
  {
    return world_->getItem(gamePosition.getPosition(), position.getStackPosition());
  }

  if (gamePosition.isInventory())
  {
    auto& player = getPlayer(creatureId);
    return player.getEquipment().getItem(gamePosition.getInventorySlot());
  }

  if (gamePosition.isContainer())
  {
    return containerManager_.getItem(getPlayerCtrl(creatureId),
                                     gamePosition.getContainerId(),
                                     gamePosition.getContainerSlot());
  }

  LOG_ERROR("%s: GamePosition: %s, is invalid", __func__, gamePosition.toString().c_str());
  return nullptr;
}

bool GameEngine::canAddItem(CreatureId creatureId, const GamePosition& position, const Item& item, int count) const
{
  // TODO(simon): count

  if (position.isPosition())
  {
    return world_->canAddItem(item, position.getPosition());
  }
  else if (position.isInventory())
  {
    return getPlayer(creatureId).getEquipment().canAddItem(item, position.getInventorySlot());
  }
  else if (position.isContainer())
  {
    // TODO(simon): check capacity of Player if root Container is in Player inventory
    return containerManager_.canAddItem(getPlayerCtrl(creatureId),
                                        position.getContainerId(),
                                        position.getContainerSlot(),
                                        item);
  }

  LOG_ERROR("%s: invalid position: %s", __func__, position.toString().c_str());
  return false;
}

void GameEngine::removeItem(CreatureId creatureId, const ItemPosition& position, int count)
{
  // TODO(simon): count
  // TODO(simon): verify itemId? verify success of removal?

  if (position.getGamePosition().isPosition())
  {
    world_->removeItem(position.getItemId(), 0, position.getGamePosition().getPosition(), position.getStackPosition());
  }
  else if (position.getGamePosition().isInventory())
  {
    auto& player = getPlayer(creatureId);
    player.getEquipment().removeItem(position.getItemId(), position.getGamePosition().getInventorySlot());
    getPlayerCtrl(creatureId)->onEquipmentUpdated(player, position.getGamePosition().getInventorySlot());
  }
  else if (position.getGamePosition().isContainer())
  {
    containerManager_.removeItem(getPlayerCtrl(creatureId),
                                 position.getGamePosition().getContainerId(),
                                 position.getGamePosition().getContainerSlot());
  }
}

void GameEngine::addItem(CreatureId creatureId, const GamePosition& position, const Item& item, int count)
{
  // TODO(simon): count
  // TODO(simon): verify success? But how do we handle failure, rollback?
  if (position.isPosition())
  {
    world_->addItem(item, position.getPosition());
  }
  else if (position.isInventory())
  {
    auto& player = getPlayer(creatureId);
    player.getEquipment().addItem(item, position.getInventorySlot());
    getPlayerCtrl(creatureId)->onEquipmentUpdated(player, position.getInventorySlot());
  }
  else if (position.isContainer())
  {
    // Note: We cannot assume that the item is added to the container referenced in position
    //       If the containerSlot points to a container-item than the item will be added
    //       to that inner container
    containerManager_.addItem(getPlayerCtrl(creatureId),
                              position.getContainerId(),
                              position.getContainerSlot(),
                              item);
  }
}
