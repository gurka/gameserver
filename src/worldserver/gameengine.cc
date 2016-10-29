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
#include <sstream>
#include <utility>

#include "player.h"
#include "playerctrl.h"
#include "creature.h"
#include "creaturectrl.h"
#include "item.h"
#include "npcctrl.h"
#include "position.h"
#include "world.h"
#include "worldfactory.h"
#include "logger.h"

GameEngine::GameEngine(boost::asio::io_service* io_service,
                       const std::string& loginMessage,
                       const std::string& dataFilename,
                       const std::string& itemsFilename,
                       const std::string& worldFilename)
  : state_(INITIALIZED),
    taskQueue_(io_service, std::bind(&GameEngine::onTask, this, std::placeholders::_1)),
    loginMessage_(loginMessage),
    world_(WorldFactory::createWorld(dataFilename, itemsFilename, worldFilename))
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
    LOG_ERROR("%s: GameEngine is already running", __func__);
    return false;
  }

  // Check so that world was created successfully
  if (!world_)
  {
    LOG_DEBUG("%s: World could not be loaded", __func__);
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
    LOG_ERROR("%s: GameEngine is already stopped", __func__);
    return false;
  }
}

CreatureId GameEngine::playerSpawn(const std::string& name, const std::function<void(const OutgoingPacket&)>& sendPacket)
{
  // Create Player and PlayerCtrl here
  Player player{name};
  PlayerCtrl playerCtrl{world_.get(), player.getCreatureId(), sendPacket};

  auto creatureId = player.getCreatureId();

  players_.insert({creatureId, std::move(player)});
  playerCtrls_.insert({creatureId, std::move(playerCtrl)});

  addTask(&GameEngine::playerSpawnInternal, creatureId);

  return creatureId;
}

void GameEngine::playerDespawn(CreatureId creatureId)
{
  addTask(&GameEngine::playerDespawnInternal, creatureId);
}

void GameEngine::playerMove(CreatureId creatureId, Direction direction)
{
  addTask(&GameEngine::playerMoveInternal, creatureId, direction);
}

void GameEngine::playerMovePath(CreatureId creatureId, const std::deque<Direction>& moves)
{
  addTask(&GameEngine::playerMovePathInternal, creatureId, moves);
}

void GameEngine::playerCancelMove(CreatureId creatureId)
{
  addTask(&GameEngine::playerCancelMoveInternal, creatureId);
}

void GameEngine::playerTurn(CreatureId creatureId, Direction direction)
{
  addTask(&GameEngine::playerMoveInternal, creatureId, direction);
}

void GameEngine::playerSay(CreatureId creatureId, uint8_t type, const std::string& message,
                           const std::string& receiver, uint16_t channelId)
{
  addTask(&GameEngine::playerSayInternal, creatureId, type, message, receiver, channelId);
}

void GameEngine::playerMoveItemFromPosToPos(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                                            int itemId, int count, const Position& toPosition)
{
  addTask(&GameEngine::playerMoveItemFromPosToPosInternal, creatureId, fromPosition, fromStackPos, itemId, count, toPosition);
}

void GameEngine::playerMoveItemFromPosToInv(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                                            int itemId, int count, int toInventoryId)
{
  addTask(&GameEngine::playerMoveItemFromPosToInvInternal, creatureId, fromPosition, fromStackPos, itemId, count, toInventoryId);
}

void GameEngine::playerMoveItemFromInvToPos(CreatureId creatureId, int fromInventoryId, int itemId, int count, const Position& toPosition)
{
  addTask(&GameEngine::playerMoveItemFromInvToPosInternal, creatureId, fromInventoryId, itemId, count, toPosition);
}

void GameEngine::playerMoveItemFromInvToInv(CreatureId creatureId, int fromInventoryId, int itemId, int count, int toInventoryId)
{
  addTask(&GameEngine::playerMoveItemFromInvToInvInternal, creatureId, fromInventoryId, itemId, count, toInventoryId);
}

void GameEngine::playerUseInvItem(CreatureId creatureId, int itemId, int inventoryIndex)
{
  addTask(&GameEngine::playerUseInvItemInternal, creatureId, itemId, inventoryIndex);
}

void GameEngine::playerUsePosItem(CreatureId creatureId, int itemId, const Position& position, int stackPos)
{
  addTask(&GameEngine::playerUsePosItemInternal, creatureId, itemId, position, stackPos);
}

void GameEngine::playerLookAt(CreatureId creatureId, const Position& position, ItemId itemId)
{
  addTask(&GameEngine::playerLookAtInternal, creatureId, position, itemId);
}

void GameEngine::playerSpawnInternal(CreatureId creatureId)
{
  auto& player = getPlayer(creatureId);
  auto& playerCtrl = getPlayerCtrl(creatureId);

  LOG_INFO("playerSpawn(): Spawn player: %s", player.getName().c_str());

  // adjustedPosition is the position where the creature actually spawned
  // i.e., if there is a creature already at the given position
  Position position(222, 222, 7);
  auto adjustedPosition = world_->addCreature(&player, &playerCtrl, position);
  if (adjustedPosition == Position::INVALID)
  {
    LOG_DEBUG("%s: Could not spawn player", __func__);
    // TODO(gurka): playerCtrl.disconnectPlayer();
    return;
  }
  playerCtrl.onPlayerSpawn(player, adjustedPosition, loginMessage_);
}

void GameEngine::playerDespawnInternal(CreatureId creatureId)
{
  LOG_INFO("playerDespawn(): Despawn player, creature id: %d", creatureId);
  world_->removeCreature(creatureId);

  // Remove Player and PlayerCtrl
  players_.erase(creatureId);
  playerCtrls_.erase(creatureId);
}

void GameEngine::playerMoveInternal(CreatureId creatureId, Direction direction)
{
  auto& playerCtrl = getPlayerCtrl(creatureId);
  auto nextWalkTime = playerCtrl.getNextWalkTime();
  auto now = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());

  if (nextWalkTime <= now)
  {
    LOG_DEBUG("%s: Player move now, creature id: %d", __func__, creatureId);
    World::ReturnCode rc = world_->creatureMove(creatureId, direction);
    if (rc == World::ReturnCode::THERE_IS_NO_ROOM)
    {
      getPlayerCtrl(creatureId).sendCancel("There is no room.");
    }
  }
  else
  {
    LOG_DEBUG("%s: Player move delayed, creature id: %d", __func__, creatureId);
    auto creatureMoveFunc = [this, creatureId, direction]
    {
      World::ReturnCode rc = world_->creatureMove(creatureId, direction);
      if (rc == World::ReturnCode::THERE_IS_NO_ROOM)
      {
        getPlayerCtrl(creatureId).sendCancel("There is no room.");
      }
    };
    taskQueue_.addTask(creatureMoveFunc, nextWalkTime);
  }
}

void GameEngine::playerMovePathInternal(CreatureId creatureId, const std::deque<Direction>& path)
{
  // TODO(gurka): This is garbage. We need to be able to cancel tasks, in case this function is called twice
  // This probably true for many more functions that isn't implemented yet

  /*
  if (!path.empty())
  {
    getPlayerCtrl(creatureId).queueMoves(path);
  }

  auto& playerCtrl = getPlayerCtrl(creatureId);

  if (!playerCtrl.hasQueuedMove())
  {
    return;
  }

  auto nextWalkTime = playerCtrl.getNextWalkTime();
  auto now = boost::posix_time::ptime(boost::posix_time::microsec_clock::local_time());

  if (nextWalkTime <= now)
  {
    LOG_INFO("%s: Player move, creature id: %d", __func__, creatureId);
    world_->creatureMove(creatureId, playerCtrl.getNextQueuedMove());
  }

  taskQueue_.addTask(std::bind(&GameEngine::playerMovePathInternal, this, creatureId, std::deque<Direction>{}), playerCtrl.getNextWalkTime());
  */
}

void GameEngine::playerCancelMoveInternal(CreatureId creatureId)
{
  getPlayerCtrl(creatureId).cancelMove();
}

void GameEngine::playerTurnInternal(CreatureId creatureId, Direction direction)
{
  LOG_INFO("playerTurn(): Player turn, creature id: %d", creatureId);
  world_->creatureTurn(creatureId, direction);
}

void GameEngine::playerSayInternal(CreatureId creatureId, uint8_t type, const std::string& message,
                                   const std::string& receiver, uint16_t channelId)
{
  LOG_INFO("%s: creatureId: %d, message: %s", __func__, creatureId, message.c_str());

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

      getPlayerCtrl(creatureId).sendTextMessage(oss.str());
    }
    else if (command == "put")
    {
      std::istringstream iss(option);
      int itemId = 0;
      iss >> itemId;

      if (itemId < 100 || itemId > 2381)
      {
        getPlayerCtrl(creatureId).sendTextMessage("Invalid itemId");
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
      getPlayerCtrl(creatureId).sendTextMessage("Invalid command");
    }
  }
  else
  {
    world_->creatureSay(creatureId, message);
  }
}

void GameEngine::playerMoveItemFromPosToPosInternal(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                                                    int itemId, int count, const Position& toPosition)
{
  LOG_INFO("playerMoveItem(): Move Item from Tile to Tile, creature id: %d, from: %s, stackPos: %d, itemId: %d, count: %d, to: %s",
             creatureId, fromPosition.toString().c_str(), fromStackPos, itemId, count, toPosition.toString().c_str());

  World::ReturnCode rc = world_->moveItem(creatureId, fromPosition, fromStackPos, itemId, count, toPosition);

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

    case World::ReturnCode::THERE_IS_NO_ROOM:
    {
      getPlayerCtrl(creatureId).sendCancel("There is no room.");
      break;
    }

    default:
    {
      // TODO(gurka): Disconnect player?
      break;
    }
  }
}

void GameEngine::playerMoveItemFromPosToInvInternal(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                                                    int itemId, int count, int toInventoryId)
{
  LOG_INFO("playerMoveItem(): Move Item from Tile to Inventory, creature id: %d, from: %s, stackPos: %d, itemId: %d, count: %d, toInventoryId: %d",
             creatureId, fromPosition.toString().c_str(), fromStackPos, itemId, count, toInventoryId);

  auto& player = getPlayer(creatureId);
  auto& playerCtrl = getPlayerCtrl(creatureId);

  // Check if the player can reach the fromPosition
  if (!world_->creatureCanReach(creatureId, fromPosition))
  {
    playerCtrl.sendCancel("You are too far away.");
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
    playerCtrl.sendCancel("You cannot equip that object.");
    return;
  }

  // Remove the Item from the fromTile
  World::ReturnCode rc = world_->removeItem(itemId, count, fromPosition, fromStackPos);
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
}

void GameEngine::playerMoveItemFromInvToPosInternal(CreatureId creatureId, int fromInventoryId, int itemId, int count, const Position& toPosition)
{
  LOG_INFO("playerMoveItem(): Move Item from Inventory to Tile, creature id: %d, from: %d, itemId: %d, count: %d, to: %s",
             creatureId, fromInventoryId, itemId, count, toPosition.toString().c_str());

  auto& player = getPlayer(creatureId);
  auto& playerCtrl = getPlayerCtrl(creatureId);
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
    playerCtrl.sendCancel("There is no room.");
    return;
  }

  // Remove the Item from the inventory slot
  if (!equipment.removeItem(item, fromInventoryId))
  {
    LOG_ERROR("playerMoveItem(): Could not remove item %d from inventory slot %d", itemId, fromInventoryId);
    return;
  }

  playerCtrl.onEquipmentUpdated(player, fromInventoryId);

  // Add the Item to the toPosition
  world_->addItem(item, toPosition);
}

void GameEngine::playerMoveItemFromInvToInvInternal(CreatureId creatureId, int fromInventoryId, int itemId, int count, int toInventoryId)
{
  LOG_INFO("playerMoveItem(): Move Item from Inventory to Inventory, creature id: %d, from: %d, itemId: %d, count: %d, to: %d",
             creatureId, fromInventoryId, itemId, count, toInventoryId);

  auto& player = getPlayer(creatureId);
  auto& playerCtrl = getPlayerCtrl(creatureId);
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
    playerCtrl.sendCancel("You cannot equip that object.");
    return;
  }

  // Remove the Item from the fromInventoryId
  if (!equipment.removeItem(item, fromInventoryId))
  {
    LOG_ERROR("playerMoveItem(): Could not remove item %d from inventory slot %d", itemId, fromInventoryId);
    return;
  }

  // Add the Item to the to-inventory slot
  equipment.addItem(item, toInventoryId);

  playerCtrl.onEquipmentUpdated(player, fromInventoryId);
  playerCtrl.onEquipmentUpdated(player, toInventoryId);
}

void GameEngine::playerUseInvItemInternal(CreatureId creatureId, int itemId, int inventoryIndex)
{
  LOG_INFO("playerUseItem(): Use Item in inventory, creature id: %d, itemId: %d, inventoryIndex: %d",
             creatureId, itemId, inventoryIndex);
  //  world_->useItem(creatureId, itemId, inventoryIndex);
}

void GameEngine::playerUsePosItemInternal(CreatureId creatureId, int itemId, const Position& position, int stackPos)
{
  LOG_INFO("playerUseItem(): Use Item at position, creature id: %d, itemId: %d, position: %s, stackPos: %d",
             creatureId, itemId, position.toString().c_str(), stackPos);
  //  world_->useItem(creatureId, itemId, position, stackPos);
}

void GameEngine::playerLookAtInternal(CreatureId creatureId, const Position& position, ItemId itemId)
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

  getPlayerCtrl(creatureId).sendTextMessage(ss.str());
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
