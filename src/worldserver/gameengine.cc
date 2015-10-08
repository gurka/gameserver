/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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
#include <utility>

#include "player.h"
#include "playerctrl.h"
#include "world/creature.h"
#include "world/creaturectrl.h"
#include "world/item.h"
#include "world/npcctrl.h"
#include "world/position.h"
#include "world/world.h"
#include "logger.h"

GameEngine::GameEngine(boost::asio::io_service* io_service, const std::string& motd, const std::string& dataFilename)
  : state_(INITIALIZED),
    taskQueue_(io_service, std::bind(&GameEngine::onTask, this, std::placeholders::_1)),
    world_(motd, dataFilename)
{
}

GameEngine::~GameEngine()
{
  if (state_ == RUNNING)
  {
    stop();
  }
}

bool GameEngine::start()
{
  if (state_ == RUNNING)
  {
    LOG_ERROR("GameEngine is already running");
    return false;
  }

  if (!world_.initialize())
  {
    LOG_ERROR("Could not initialize World");
    return false;
  }

  state_ = RUNNING;

  return true;
}

bool GameEngine::stop()
{
  if (state_ == RUNNING)
  {
    state_ = CLOSING;
    return true;
  }
  else
  {
    LOG_ERROR("GameEngine is already stopped");
    return false;
  }
}

CreatureId GameEngine::playerSpawn(const std::string& name, const std::function<void(const OutgoingPacket&)>& sendPacket)
{
  // Create Player and PlayerCtrl here
  std::unique_ptr<Player> player(new Player(name));
  std::unique_ptr<PlayerCtrl> playerCtrl(new PlayerCtrl(&world_, player->getCreatureId(), sendPacket));

  auto creatureId = player->getCreatureId();

  players_.insert(std::make_pair(creatureId, std::move(player)));
  playerCtrls_.insert(std::make_pair(creatureId, std::move(playerCtrl)));

  auto playerSpawnFunc = [this, creatureId]()
  {
    auto& player = getPlayer(creatureId);
    auto& playerCtrl = getPlayerCtrl(creatureId);

    LOG_INFO("playerSpawn(): Spawn player: %s", player.getName().c_str());

    Position position(222, 222, 7);
    world_.addCreature(players_.at(creatureId).get(), playerCtrls_.at(creatureId).get(), position);

    playerCtrl.onPlayerSpawn(player, position);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerSpawnFunc, expire);

  return creatureId;
}

bool GameEngine::playerDespawn(CreatureId creatureId)
{
  auto playerDespawnFunc = [this, creatureId]()
  {
    LOG_INFO("playerDespawn(): Despawn player, creature id: %d", creatureId);
    world_.removeCreature(creatureId);

    // Remove Player and PlayerCtrl
    players_.erase(creatureId);
    playerCtrls_.erase(creatureId);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerDespawnFunc, expire);

  // Logout is always OK
  return true;
}

void GameEngine::playerMove(CreatureId creatureId, Direction direction)
{
  auto playerMoveFunc = [this, creatureId, direction]()
  {
    LOG_INFO("playerMove(): Player move, creature id: %d", creatureId);
    world_.creatureMove(creatureId, direction);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerMoveFunc, expire);
}

void GameEngine::playerMove(CreatureId creatureId, const std::list<Direction>& path)
{
  auto playerMoveFunc = [this, creatureId, path]()
  {
    LOG_INFO("playerMove(): Player move path, creature id: %d, moves left: %d", creatureId, path.size());
    world_.creatureMove(creatureId, path.front());

    if (path.size() > 1)
    {
      std::list<Direction> newPath(path);
      newPath.erase(newPath.cbegin());
      playerMove(creatureId, newPath);
    }
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerMoveFunc, expire);
}

void GameEngine::playerTurn(CreatureId creatureId, Direction direction)
{
  auto playerTurnFunc = [this, creatureId, direction]()
  {
    LOG_INFO("playerTurn(): Player turn, creature id: %d", creatureId);
    world_.creatureTurn(creatureId, direction);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerTurnFunc, expire);
}

void GameEngine::playerSay(CreatureId creatureId, uint8_t type, const std::string& message,
                           const std::string& receiver, uint16_t channelId)
{
  auto playerSayFunc = [this, creatureId, message]()
  {
    LOG_INFO("playerSay(): Player say, creature id: %d, message: %s", creatureId, message.c_str());
    world_.creatureSay(creatureId, message);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerSayFunc, expire);
}

void GameEngine::playerMoveItem(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                                int itemId, int count, const Position& toPosition)
{
  auto playerMoveItemFunc = [this, creatureId, fromPosition, fromStackPos, itemId, count, toPosition]()
  {
    LOG_INFO("playerMoveItem(): Move Item from Tile to Tile, creature id: %d, from: %s, stackPos: %d, itemId: %d, count: %d, to: %s",
               creatureId, fromPosition.toString().c_str(), fromStackPos, itemId, count, toPosition.toString().c_str());

    World::ReturnCode rc = world_.moveItem(creatureId, fromPosition, fromStackPos, itemId, count, toPosition);

    switch (rc)
    {
      case World::ReturnCode::OK:
      {
        break;
      }

      case World::ReturnCode::CANNOT_MOVE_THAT_OBJECT:
      {
        getPlayerCtrl(creatureId).sendCancel("You cannot move that object.");
        break;
      }

      case World::ReturnCode::CANNOT_REACH_THAT_OBJECT:
      {
        getPlayerCtrl(creatureId).sendCancel("You are too far away.");
        break;
      }

      default:
      {
        // TODO(gurka): Disconnect player?
        break;
      }
    }
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerMoveItemFunc, expire);
}

void GameEngine::playerMoveItem(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                                int itemId, int count, int toInventoryId)
{
  auto playerMoveItemFunc = [this, creatureId, fromPosition, fromStackPos, itemId, count, toInventoryId]()
  {
    LOG_INFO("playerMoveItem(): Move Item from Tile to Inventory, creature id: %d, from: %s, stackPos: %d, itemId: %d, count: %d, toInventoryId: %d",
               creatureId, fromPosition.toString().c_str(), fromStackPos, itemId, count, toInventoryId);

    auto& player = getPlayer(creatureId);
    auto& playerCtrl = getPlayerCtrl(creatureId);
    Item item(itemId);

    // Check if the player can reach the fromPosition
    if (!world_.creatureCanReach(creatureId, fromPosition))
    {
      playerCtrl.sendCancel("You cannot move that object.");
      return;
    }

    // Check if we can add the Item to that inventory slot
    auto& equipment = player.getEquipment();
    if (!equipment.canAddItem(item, toInventoryId))
    {
      playerCtrl.sendCancel("You cannot equip that object.");
      return;
    }

    // Try to remove the Item from the fromTile
    World::ReturnCode rc = world_.removeItem(itemId, count, fromPosition, fromStackPos);
    if (rc != World::ReturnCode::OK)
    {
      LOG_ERROR("playerMoveItem(): Could not remove item %d (count %d) from %s (stackpos: %d)",
                  itemId, count, fromPosition.toString().c_str(), fromStackPos);
      // TODO(gurka): Disconnect player?
      return;
    }

    // Add the Item to the inventory
    equipment.addItem(item, toInventoryId);

    playerCtrl.onEquipmentUpdated(player, toInventoryId);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerMoveItemFunc, expire);
}

void GameEngine::playerMoveItem(CreatureId creatureId, int fromInventoryId, int itemId, int count, const Position& toPosition)
{
  auto playerMoveItemFunc = [this, creatureId, fromInventoryId, itemId, count, toPosition]()
  {
    LOG_INFO("playerMoveItem(): Move Item from Inventory to Tile, creature id: %d, from: %d, itemId: %d, count: %d, to: %s",
               creatureId, fromInventoryId, itemId, count, toPosition.toString().c_str());

    auto& player = getPlayer(creatureId);
    auto& playerCtrl = getPlayerCtrl(creatureId);
    Item item(itemId);

    // First check if the player can throw the Item to the toPosition
    if (!world_.creatureCanThrowTo(creatureId, toPosition))
    {
      playerCtrl.sendCancel("There is no room.");
      return;
    }

    // Try to remove the Item from the inventory slot
    auto& equipment = player.getEquipment();
    if (!equipment.removeItem(item, fromInventoryId))
    {
      LOG_ERROR("playerMoveItem(): Could not remove item %d from inventory slot %d", itemId, fromInventoryId);
      return;
    }

    playerCtrl.onEquipmentUpdated(player, fromInventoryId);

    // Add the Item to the toPosition
    world_.addItem(itemId, count, toPosition);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerMoveItemFunc, expire);
}

void GameEngine::playerMoveItem(CreatureId creatureId, int fromInventoryId, int itemId, int count, int toInventoryId)
{
  auto playerMoveItemFunc = [this, creatureId, fromInventoryId, itemId, count, toInventoryId]()
  {
    LOG_INFO("playerMoveItem(): Move Item from Inventory to Inventory, creature id: %d, from: %d, itemId: %d, count: %d, to: %d",
               creatureId, fromInventoryId, itemId, count, toInventoryId);

    auto& player = getPlayer(creatureId);
    auto& playerCtrl = getPlayerCtrl(creatureId);
    Item item(itemId);

    // TODO(gurka): Count

    // Check if we can add the Item to the to-inventory slot
    auto& equipment = player.getEquipment();
    if (!equipment.canAddItem(item, toInventoryId))
    {
      playerCtrl.sendCancel("You cannot equip that object.");
      return;
    }

    // Try to remove the Item from the from-inventory slot
    if (!equipment.removeItem(item, fromInventoryId))
    {
      LOG_ERROR("playerMoveItem(): Could not remove item %d from inventory slot %d", itemId, fromInventoryId);
      return;
    }

    // Add the Item to the to-inventory slot
    equipment.addItem(item, toInventoryId);

    playerCtrl.onEquipmentUpdated(player, fromInventoryId);
    playerCtrl.onEquipmentUpdated(player, toInventoryId);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerMoveItemFunc, expire);
}

void GameEngine::playerUseItem(CreatureId creatureId, int itemId, int inventoryIndex)
{
  auto playerUseItemFunc = [this, creatureId, itemId, inventoryIndex]()
  {
    LOG_INFO("playerUseItem(): Use Item in inventory, creature id: %d, itemId: %d, inventoryIndex: %d",
               creatureId, itemId, inventoryIndex);
    //  world_.useItem(creatureId, itemId, inventoryIndex);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerUseItemFunc, expire);
}

void GameEngine::playerUseItem(CreatureId creatureId, int itemId, const Position& position, int stackPos)
{
  auto playerUseItemFunc = [this, creatureId, itemId, position, stackPos]()
  {
    LOG_INFO("playerUseItem(): Use Item at position, creature id: %d, itemId: %d, position: %s, stackPos: %d",
               creatureId, itemId, position.toString().c_str(), stackPos);
    //  world_.useItem(creatureId, itemId, position, stackPos);
  };

  auto expire = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());
  taskQueue_.addTask(playerUseItemFunc, expire);
}

void GameEngine::playerLookAt(CreatureId creatureId, const Position& position, ItemId itemId)
{
  std::ostringstream ss;

  if (itemId == 99)
  {
    ss << "You are looking at a creature.";
  }
  else
  {
    Item item(itemId);
    if (item.getName().size() > 0)
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

  //  world_.sendTextMessage(creatureId, ss.str());
}

void GameEngine::onTask(const TaskFunction& task)
{
  switch (state_)
  {
    case RUNNING:
    {
      LOG_INFO("onTask(): Calling TaskFunction!");
      task();
      break;
    }

    case CLOSING:
    {
      LOG_INFO("onTask(): State is CLOSING, not executing task.");
      // TODO(gurka): taskQueue_.stop();
      state_ = CLOSED;
      break;
    }

    case CLOSED:
    {
      LOG_INFO("onTask(): State is CLOSED, not executing task.");
      break;
    }

    default:
    {
      LOG_ERROR("onTask(): Unknown state: %d", state_);
      break;
    }
  }
}
