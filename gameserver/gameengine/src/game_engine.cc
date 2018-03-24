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
  playerPlayerCtrl_.emplace(creatureId, PlayerPlayerCtrl{std::move(newPlayer), player_ctrl, {}});

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
  world_->removeCreature(creatureId);

  // Remove Player and PlayerCtrl
  playerPlayerCtrl_.erase(creatureId);

  // Remove any queued tasks for this player
  gameEngineQueue_->cancelAllTasks(creatureId);
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

void GameEngine::moveItem(CreatureId creatureId,
                          const ItemPosition& fromPosition,
                          int itemId,
                          int fromStackPos,
                          const ItemPosition& toPosition,
                          int count)
{
  getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Not yet implemented.");
}

void GameEngine::useItem(CreatureId creatureId,
                         const ItemPosition& position,
                         int itemId,
                         int stackPosition,
                         int newContainerId)
{
  int parentContainerId = Container::INVALID_ID;
  Item* item = nullptr;

  // Try to retrieve the item without world context
  if (position.isInventorySlot())
  {
    // Using an item in inventory doesn't need world context
    auto inventorySlot = position.getInventorySlot();

    // Get the item
    auto& player = getPlayer(creatureId);
    auto& inventory = player.getEquipment();
    if (!inventory.hasItem(inventorySlot))
    {
      LOG_ERROR("%s: no Item in given inventorySlot", __func__);
      return;
    }

    item = &inventory.getItem(inventorySlot);
  }
  else if (position.isContainer())
  {
    // Using an item in a container doesn't need world context if the
    // container is in the player's inventory
    const auto clientContainerId = position.getContainerId();

    // Map client contianer id to global container id and get the container
    const auto containerId = playerPlayerCtrl_.at(creatureId).openContainers[clientContainerId];
    auto& container = containerManager_.getContainer(containerId);
    if (container.id == Container::INVALID_ID)
    {
      LOG_ERROR("%s: clientContainerId: %d, containerId: %d, container is invalid",
                __func__,
                clientContainerId,
                containerId);
      return;
    }

    if (container.rootContainerId == Container::PARENT_IS_PLAYER)
    {
      // Make sure that the container slot is valid
      const auto containerSlot = position.getContainerSlot();
      if (static_cast<int>(container.items.size()) <= containerSlot)
      {
        LOG_ERROR("%s: clientContainerId: %d, containerId: %d, containerSlot: %d, items.size: %u, out of range",
                  __func__,
                  clientContainerId,
                  containerId,
                  containerSlot,
                  container.items.size());
        return;
      }

      // Set parent containerId and get the item
      parentContainerId = containerId;
      item = &container.items[containerSlot];
    }
  }
  else
  {
    // TODO(simon): needs world context!
    getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Not yet implemented.");
  }

  if (!item)
  {
    // Could not get the item without world context
    getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Not yet implemented.");
    return;
  }

  if (item->getItemId() != itemId)
  {
    LOG_ERROR("%s: expected itemId: %d actual itemId: %d", __func__, itemId, item->getItemId());
    return;
  }

  // Handle containers
  if (item->isContainer())
  {
    if (item->getContainerId() == Container::INVALID_ID)
    {
      // TODO(simon): create new container with Position as parent when applicable
      const auto containerId = parentContainerId == Container::INVALID_ID ?
                               containerManager_.createNewContainer(item->getItemId()) :
                               containerManager_.createNewContainer(item->getItemId(), parentContainerId);
      item->setContainerId(containerId);
      LOG_DEBUG("%s: created new Container with id: %d", __func__, containerId);
    }

    const auto& container = containerManager_.getContainer(item->getContainerId());

    // Check if player already have this container open
    auto& openContainers = playerPlayerCtrl_.at(creatureId).openContainers;
    int clientContainerId = -1;
    for (auto i = 0u; i < openContainers.size(); i++)
    {
      if (openContainers[i] == container.id)
      {
        clientContainerId = i;
        break;
      }
    }

    if (clientContainerId == -1)
    {
      // Container not yet open, so open it
      clientContainerId = newContainerId;
      openContainers[clientContainerId] = container.id;
      containerManager_.addPlayer(container.id, creatureId);
      getPlayerCtrl(creatureId)->onOpenContainer(clientContainerId, container);
    }
    else
    {
      // Container already open, so close it
      getPlayerCtrl(creatureId)->onCloseContainer(clientContainerId);
    }
  }
  else
  {
    getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Not yet implemented.");
  }
}

void GameEngine::lookAt(CreatureId creatureId, const ItemPosition& position, int itemId, int stackPosition)
{
  getPlayerCtrl(creatureId)->sendTextMessage(0x13, "Not yet implemented.");
}

void GameEngine::closeContainer(CreatureId creatureId, int clientContainerId)
{
  LOG_DEBUG("%s: creatureId: %d clientContainerId: %d", __func__, creatureId, clientContainerId);

  // Verify that the Player actually have this container open
  auto& openContainers = playerPlayerCtrl_.at(creatureId).openContainers;

  if (static_cast<int>(openContainers.size()) <= clientContainerId ||
      openContainers[clientContainerId] == Container::INVALID_ID)
  {
    LOG_ERROR("%s: player does not have the given Container open", __func__);
    return;
  }

  // Remove this Player from Container's list of Players
  containerManager_.removePlayer(openContainers[clientContainerId], creatureId);

  // Remove the local to global container id mapping
  openContainers[clientContainerId] = Container::INVALID_ID;

  // Send onCloseContainer to player again (???)
//  getPlayerCtrl(creatureId)->onCloseContainer(clientContainerId);
}
