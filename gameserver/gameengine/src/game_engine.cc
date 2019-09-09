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

#include "game_engine_queue.h"
#include "player.h"
#include "player_ctrl.h"
#include "creature.h"
#include "creature_ctrl.h"
#include "item.h"
#include "item_manager.h"
#include "container_manager.h"
#include "position.h"
#include "world_factory.h"
#include "logger.h"
#include "tick.h"

namespace
{

struct RecursiveTask
{
  explicit RecursiveTask(const std::function<void(const RecursiveTask&, GameEngine* gameEngine)>& func)
    : func_(func)
  {
  }

  void operator()(GameEngine* gameEngine) const
  {
    func_(*this, gameEngine);
  }

  std::function<void(const RecursiveTask&, GameEngine* gameEngine)> func_;
};

}  // namespace

GameEngine::GameEngine()
    : playerData_(),
      itemManager_(),
      world_(),
      gameEngineQueue_(nullptr),
      loginMessage_(),
      containerManager_()
{
}
GameEngine::~GameEngine() = default;

bool GameEngine::init(GameEngineQueue* gameEngineQueue,
                      const std::string& loginMessage,
                      const std::string& dataFilename,
                      const std::string& itemsFilename,
                      const std::string& worldFilename)
{
  gameEngineQueue_ = gameEngineQueue;
  loginMessage_ = loginMessage;

  // Load ItemManager
  itemManager_ = std::make_unique<ItemManager>();
  if (!itemManager_->loadItemTypes(dataFilename, itemsFilename))
  {
    LOG_ERROR("%s: could not load ItemManager", __func__);
    return false;
  }

  // Load World
  world_ = WorldFactory::createWorld(worldFilename, itemManager_.get());
  if (!world_)
  {
    LOG_ERROR("%s: could not load World", __func__);
    return false;
  }

  // Create ContainerManager
  containerManager_ = std::make_unique<ContainerManager>();

  return true;
}

bool GameEngine::spawn(const std::string& name, PlayerCtrl* player_ctrl)
{
  // Create the Player
  Player newPlayer{name};
  auto creatureId = newPlayer.getCreatureId();

  // Store the Player and the PlayerCtrl
  playerData_.emplace(std::piecewise_construct,
                      std::forward_as_tuple(creatureId),
                      std::forward_as_tuple(std::move(newPlayer), player_ctrl));

  // Get the Player again, since newPlayed is moved from
  auto& player = getPlayerData(creatureId).player;

  LOG_DEBUG("%s: Spawn player: %s", __func__, player.getName().c_str());

  // Tell PlayerCtrl its CreatureId
  player_ctrl->setPlayerId(player.getCreatureId());

  // Spawn the player
  auto rc = world_->addCreature(&player, player_ctrl, Position(208, 208, 7));
  if (rc != World::ReturnCode::OK)
  {
    LOG_DEBUG("%s: could not spawn player", __func__);
    containerManager_->playerDespawn(player_ctrl);
    player_ctrl->setPlayerId(Creature::INVALID_ID);
    playerData_.erase(creatureId);
    return false;
  }

  player_ctrl->sendTextMessage(0x11, loginMessage_);
  return true;
}

void GameEngine::despawn(CreatureId creatureId)
{
  LOG_DEBUG("%s: Despawn player, creature id: %d", __func__, creatureId);

  // Inform ContainerManager
  containerManager_->playerDespawn(getPlayerData(creatureId).player_ctrl);

  // Remove any queued tasks for this player
  gameEngineQueue_->cancelAllTasks(creatureId);

  // Finally despawn the player from the World
  // Note: this will free the PlayerCtrl, but requires Player to still be allocated
  world_->removeCreature(creatureId);

  // Remove Player and PlayerCtrl
  playerData_.erase(creatureId);
}

void GameEngine::move(CreatureId creatureId, Direction direction)
{
  LOG_DEBUG("%s: creature id: %d", __func__, creatureId);

  auto* player_ctrl = getPlayerData(creatureId).player_ctrl;

  auto rc = world_->creatureMove(creatureId, direction);
  if (rc == World::ReturnCode::MAY_NOT_MOVE_YET)
  {
    LOG_DEBUG("%s: player move delayed, creature id: %d", __func__, creatureId);
    const auto& creature = world_->getCreature(creatureId);
    gameEngineQueue_->addTask(creatureId,
                              creature.getNextWalkTick() - Tick::now(),
                              [this, creatureId, direction](GameEngine* gameEngine)
    {
      (void)gameEngine;
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
  getPlayerData(creatureId).queued_moves = std::move(path);

  const auto task = RecursiveTask([this, creatureId](const RecursiveTask& task, GameEngine* gameEngine)
  {
    (void)gameEngine;

    auto& playerData = getPlayerData(creatureId);

    // Make sure that the queued moves hasn't been canceled
    if (!playerData.queued_moves.empty())
    {
      const auto rc = world_->creatureMove(creatureId, playerData.queued_moves.front());

      if (rc == World::ReturnCode::OK)
      {
        // Player moved, pop the move from the queue
        playerData.queued_moves.pop_front();
      }
      else if (rc != World::ReturnCode::MAY_NOT_MOVE_YET)
      {
        // If we neither got OK nor MAY_NOT_MOVE_YET: stop here and cancel all queued moves
        cancelMove(creatureId);
      }

      if (!playerData.queued_moves.empty())
      {
        // If there are more queued moves, e.g. we moved but there are more moves or we were not allowed
        // to move yet, add a new task
        gameEngineQueue_->addTask(creatureId, playerData.player.getNextWalkTick() - Tick::now(), task);
      }
    }
  });

  task(this);
}

void GameEngine::cancelMove(CreatureId creatureId)
{
  LOG_DEBUG("%s: creature id: %d", __func__, creatureId);

  auto& playerData = getPlayerData(creatureId);
  if (!playerData.queued_moves.empty())
  {
    playerData.queued_moves.clear();
    playerData.player_ctrl->cancelMove();
  }

  // Don't cancel the task, just let it expire and do nothing
}

void GameEngine::turn(CreatureId creatureId, Direction direction)
{
  LOG_DEBUG("%s: Player turn, creature id: %d", __func__, creatureId);
  world_->creatureTurn(creatureId, direction);
}

void GameEngine::say(CreatureId creatureId,
                     int type,
                     const std::string& message,
                     const std::string& receiver,
                     int channelId)
{
  // TODO(simon): care about these
  (void)type;
  (void)receiver;
  (void)channelId;

  auto& playerData = getPlayerData(creatureId);

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
        position = world_->getCreaturePosition(creatureId).addDirection(playerData.player.getDirection());
      }

      const auto* tile = world_->getTile(position);

      std::ostringstream oss;
      oss << "Position: " << position.toString() << "\n";

      for (const auto& thing : tile->getThings())
      {
        if (thing.item)
        {
          oss << "Item: " << thing.item->getItemUniqueId() << "\n";
        }
        else
        {
          oss << "Creature: " << thing.creature->getCreatureId() << "\n";
        }
      }

      playerData.player_ctrl->sendTextMessage(0x13, oss.str());
    }
    else if (command == "put")
    {
      // TODO(simon): replace itemManager->createItem/getItem with world->createItem/getItem

//      std::istringstream iss(option);
//      ItemTypeId itemTypeId = 0;
//      iss >> itemTypeId;
//
//      const auto itemId = itemManager_->createItem(itemTypeId);
//
//      if (itemId == 0)  // TODO(simon): see item_manager TODO
//      {
//        playerData.player_ctrl->sendTextMessage(0x13, "Invalid itemId");
//      }
//      else
//      {
//        auto* item = itemManager_->getItem(itemId);
//        const auto position = world_->getCreaturePosition(creatureId).addDirection(playerData.player.getDirection());
//        world_->addItem(item, position);
//      }
    }
    else
    {
      playerData.player_ctrl->sendTextMessage(0x13, "Invalid command");
    }
  }
  else
  {
    world_->creatureSay(creatureId, message);
  }
}

void GameEngine::moveItem(CreatureId creatureId,
                          const ItemPosition& fromPosition,
                          const GamePosition& toPosition,
                          int count)
{
  LOG_DEBUG("%s: creatureId: %d fromPosition: %s toPosition: %s count: %d",
            __func__,
            creatureId,
            fromPosition.toString().c_str(),
            toPosition.toString().c_str(),
            count);

  auto& playerData = getPlayerData(creatureId);

  if (fromPosition.getItemTypeId() == 0x63)
  {
    // Move Creature
    playerData.player_ctrl->sendTextMessage(0x13, "Not yet implemented.");
    return;
  }

  // Verify that the Item is at fromPosition and can be moved
  if (!getItem(creatureId, fromPosition))
  {
    LOG_ERROR("%s: could not find Item at fromPosition: %s", __func__, fromPosition.toString().c_str());
    return;
  }
  auto* item = getItem(creatureId, fromPosition);

  // TODO(simon): verify that Item is movable

  // TODO(simon): check if toPosition points to a container item, and in that case change toPosition
  //              to point inside that container (if applicable)

  // Verify that the Item can be added to toPosition
  if (!canAddItem(creatureId, toPosition, *item, count))
  {
    // TODO(simon): proper error message to player
    LOG_ERROR("%s: cannot add Item to toPosition: %s", __func__, toPosition.toString().c_str());
    return;
  }

  // Remove Item from fromPosition
  removeItem(creatureId, fromPosition, count);

  // Add Item to toPosition
  addItem(creatureId, toPosition, *item, count);
}

void GameEngine::useItem(CreatureId creatureId, const ItemPosition& position, int newContainerId)
{
  LOG_DEBUG("%s: creatureId: %d position: %s newContainerId: %d",
            __func__,
            creatureId,
            position.toString().c_str(),
            newContainerId);

  auto& playerData = getPlayerData(creatureId);

  auto* item = getItem(creatureId, position);
  if (!item)
  {
    LOG_ERROR("%s: could not find Item", __func__);
    return;
  }

  // TODO(simon): verify that player is close enough, if item is in world

  if (item->getItemType().isContainer)
  {
    containerManager_->useContainer(playerData.player_ctrl, *item, position.getGamePosition(), newContainerId);
  }
}

void GameEngine::lookAt(CreatureId creatureId, const ItemPosition& position)
{
  LOG_DEBUG("%s: creatureId: %d position: %s",
            __func__,
            creatureId,
            position.toString().c_str());

  auto& playerData = getPlayerData(creatureId);

  auto* item = getItem(creatureId, position);
  if (!item)
  {
    LOG_ERROR("%s: could not find Item", __func__);
    return;
  }

  // TODO(simon): verify that player is close enough, if item is in world

  const auto& itemType = item->getItemType();

  std::ostringstream ss;

  if (!itemType.name.empty())
  {
    if (itemType.isStackable && item->getCount() > 1)
    {
      ss << "You see " << item->getCount() << " " << itemType.name << "s.";
    }
    else
    {
      ss << "You see a " << itemType.name << ".";
    }
  }
  else
  {
    ss << "You see an item with ItemTypeId: " << item->getItemTypeId() << ".";
  }

  // TODO(simon): only if standing next to the item
  if (itemType.weight != 0)
  {
    ss << "\nIt weights " << itemType.weight << " oz.";
  }

  if (!itemType.descr.empty())
  {
    ss << "\n" << itemType.descr;
  }

  playerData.player_ctrl->sendTextMessage(0x13, ss.str());
}

void GameEngine::closeContainer(CreatureId creatureId, ItemUniqueId itemUniqueId)
{
  LOG_DEBUG("%s: creatureId: %d itemUniqueId: %d", __func__, creatureId, itemUniqueId);
  containerManager_->closeContainer(getPlayerData(creatureId).player_ctrl, itemUniqueId);
}

void GameEngine::openParentContainer(CreatureId creatureId, ItemUniqueId itemUniqueId, int newContainerId)
{
  LOG_DEBUG("%s: creatureId: %d itemUniqueId: %d", __func__, creatureId, itemUniqueId);
  containerManager_->openParentContainer(getPlayerData(creatureId).player_ctrl,
                                         itemUniqueId,
                                         newContainerId);
}

const Item* GameEngine::getItem(CreatureId creatureId, const ItemPosition& position)
{
  // TODO(simon): verify ItemId
  auto& playerData = getPlayerData(creatureId);

  const auto& gamePosition = position.getGamePosition();
  if (gamePosition.isPosition())
  {
    return world_->getTile(gamePosition.getPosition())->getItem(position.getStackPosition());
  }

  if (gamePosition.isInventory())
  {
    return playerData.player.getEquipment().getItem(gamePosition.getInventorySlot());
  }

  if (gamePosition.isContainer())
  {
    return containerManager_->getItem(gamePosition.getItemUniqueId(),
                                      gamePosition.getContainerSlot());
  }

  LOG_ERROR("%s: GamePosition: %s, is invalid", __func__, gamePosition.toString().c_str());
  return nullptr;
}

bool GameEngine::canAddItem(CreatureId creatureId, const GamePosition& position, const Item& item, int count)
{
  auto& playerData = getPlayerData(creatureId);

  // TODO(simon): count
  (void)count;

  if (position.isPosition())
  {
    return world_->canAddItem(item, position.getPosition());
  }
  else if (position.isInventory())
  {
    return playerData.player.getEquipment().canAddItem(item, position.getInventorySlot());
  }
  else if (position.isContainer())
  {
    // TODO(simon): check capacity of Player if root Container is in Player inventory
    return containerManager_->canAddItem(position.getItemUniqueId(),
                                         position.getContainerSlot(),
                                         item);
  }

  LOG_ERROR("%s: invalid position: %s", __func__, position.toString().c_str());
  return false;
}

void GameEngine::removeItem(CreatureId creatureId, const ItemPosition& position, int count)
{
  auto& playerData = getPlayerData(creatureId);

  // TODO(simon): count
  (void)count;

  // TODO(simon): verify itemId? verify success of removal?

  if (position.getGamePosition().isPosition())
  {
    world_->removeItem(position.getItemTypeId(),
                       1,
                       position.getGamePosition().getPosition(),
                       position.getStackPosition());
  }
  else if (position.getGamePosition().isInventory())
  {
    playerData.player.getEquipment().removeItem(position.getItemTypeId(),
                                                position.getGamePosition().getInventorySlot());
    playerData.player_ctrl->onEquipmentUpdated(playerData.player, position.getGamePosition().getInventorySlot());
  }
  else if (position.getGamePosition().isContainer())
  {
    containerManager_->removeItem(position.getGamePosition().getItemUniqueId(),
                                  position.getGamePosition().getContainerSlot());
  }
}

void GameEngine::addItem(CreatureId creatureId, const GamePosition& position, const Item& item, int count)
{
  auto& playerData = getPlayerData(creatureId);

  // TODO(simon): count
  (void)count;

  // TODO(simon): verify success? But how do we handle failure, rollback?
  if (position.isPosition())
  {
    world_->addItem(item, position.getPosition());
  }
  else if (position.isInventory())
  {
    playerData.player.getEquipment().addItem(item, position.getInventorySlot());
    getPlayerData(creatureId).player_ctrl->onEquipmentUpdated(playerData.player, position.getInventorySlot());
  }
  else if (position.isContainer())
  {
    // Note: We cannot assume that the item is added to the container referenced in position
    //       If the containerSlot points to a container-item than the item will be added
    //       to that inner container
    containerManager_->addItem(position.getItemUniqueId(),
                               position.getContainerSlot(),
                               item);
  }

  // Handle case where a container is moved to a non-container
  if (item.getItemType().isContainer && !position.isContainer())
  {
    containerManager_->updateRootPosition(item.getItemUniqueId(), position);
  }
}
