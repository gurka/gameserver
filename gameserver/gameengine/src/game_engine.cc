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
  explicit RecursiveTask(std::function<void(const RecursiveTask&, GameEngine* game_engine)> func)
    : func(std::move(func))
  {
  }

  void operator()(GameEngine* game_engine) const
  {
    func(*this, game_engine);
  }

  std::function<void(const RecursiveTask&, GameEngine* game_engine)> func;
};

}  // namespace

GameEngine::GameEngine()
    : m_item_manager(),
      m_world(),
      m_game_engine_queue(nullptr),
      m_container_manager()
{
}
GameEngine::~GameEngine() = default;

bool GameEngine::init(GameEngineQueue* game_engine_queue,
                      const std::string& login_message,
                      const std::string& data_filename,
                      const std::string& items_filename,
                      const std::string& world_filename)
{
  m_game_engine_queue = game_engine_queue;
  m_login_message = login_message;

  // Load ItemManager
  m_item_manager = std::make_unique<ItemManager>();
  if (!m_item_manager->loadItemTypes(data_filename, items_filename))
  {
    LOG_ERROR("%s: could not load ItemManager", __func__);
    return false;
  }

  // Load World
  m_world = WorldFactory::createWorld(world_filename, m_item_manager.get());
  if (!m_world)
  {
    LOG_ERROR("%s: could not load World", __func__);
    return false;
  }

  // Create ContainerManager
  m_container_manager = std::make_unique<ContainerManager>();

  return true;
}

bool GameEngine::spawn(const std::string& name, PlayerCtrl* player_ctrl)
{
  // Create the Player
  Player new_player{name};
  auto creature_id = new_player.getCreatureId();

  // Store the Player and the PlayerCtrl
  m_player_data.emplace(std::piecewise_construct,
                      std::forward_as_tuple(creature_id),
                      std::forward_as_tuple(std::move(new_player), player_ctrl));

  // Get the Player again, since newPlayed is moved from
  auto& player = getPlayerData(creature_id).player;

  LOG_DEBUG("%s: Spawn player: %s", __func__, player.getName().c_str());

  // Tell PlayerCtrl its CreatureId
  player_ctrl->setPlayerId(player.getCreatureId());

  // Spawn the player
  auto rc = m_world->addCreature(&player, player_ctrl, Position(208, 208, 7));
  if (rc != World::ReturnCode::OK)
  {
    LOG_DEBUG("%s: could not spawn player", __func__);
    m_container_manager->playerDespawn(player_ctrl);
    player_ctrl->setPlayerId(Creature::INVALID_ID);
    m_player_data.erase(creature_id);
    return false;
  }

  player_ctrl->sendTextMessage(0x11, m_login_message);
  return true;
}

void GameEngine::despawn(CreatureId creature_id)
{
  LOG_DEBUG("%s: Despawn player, creature id: %d", __func__, creature_id);

  // Inform ContainerManager
  m_container_manager->playerDespawn(getPlayerData(creature_id).player_ctrl);

  // Remove any queued tasks for this player
  m_game_engine_queue->cancelAllTasks(creature_id);

  // Finally despawn the player from the World
  // Note: this will free the PlayerCtrl, but requires Player to still be allocated
  m_world->removeCreature(creature_id);

  // Remove Player and PlayerCtrl
  m_player_data.erase(creature_id);
}

void GameEngine::move(CreatureId creature_id, Direction direction)
{
  LOG_DEBUG("%s: creature id: %d", __func__, creature_id);

  auto* player_ctrl = getPlayerData(creature_id).player_ctrl;

  auto rc = m_world->creatureMove(creature_id, direction);
  if (rc == World::ReturnCode::MAY_NOT_MOVE_YET)
  {
    LOG_DEBUG("%s: player move delayed, creature id: %d", __func__, creature_id);
    const auto& creature = static_cast<const World*>(m_world.get())->getCreature(creature_id);
    m_game_engine_queue->addTask(creature_id,
                              creature.getNextWalkTick() - Tick::now(),
                              [this, creature_id, direction](GameEngine* game_engine)
    {
      (void)game_engine;
      move(creature_id, direction);
    });
  }
  else if (rc == World::ReturnCode::THERE_IS_NO_ROOM)
  {
    player_ctrl->sendCancel("There is no room.");
  }
}

void GameEngine::movePath(CreatureId creature_id, std::deque<Direction>&& path)
{
  getPlayerData(creature_id).queued_moves = std::move(path);

  const auto task = RecursiveTask([this, creature_id](const RecursiveTask& task, GameEngine* game_engine)
  {
    (void)game_engine;

    auto& player_data = getPlayerData(creature_id);

    // Make sure that the queued moves hasn't been canceled
    if (!player_data.queued_moves.empty())
    {
      const auto rc = m_world->creatureMove(creature_id, player_data.queued_moves.front());

      if (rc == World::ReturnCode::OK)
      {
        // Player moved, pop the move from the queue
        player_data.queued_moves.pop_front();
      }
      else if (rc != World::ReturnCode::MAY_NOT_MOVE_YET)
      {
        // If we neither got OK nor MAY_NOT_MOVE_YET: stop here and cancel all queued moves
        cancelMove(creature_id);
      }

      if (!player_data.queued_moves.empty())
      {
        // If there are more queued moves, e.g. we moved but there are more moves or we were not allowed
        // to move yet, add a new task
        m_game_engine_queue->addTask(creature_id, player_data.player.getNextWalkTick() - Tick::now(), task);
      }
    }
  });

  task(this);
}

void GameEngine::cancelMove(CreatureId creature_id)
{
  LOG_DEBUG("%s: creature id: %d", __func__, creature_id);

  auto& player_data = getPlayerData(creature_id);
  if (!player_data.queued_moves.empty())
  {
    player_data.queued_moves.clear();
    player_data.player_ctrl->cancelMove();
  }

  // Don't cancel the task, just let it expire and do nothing
}

void GameEngine::turn(CreatureId creature_id, Direction direction)
{
  LOG_DEBUG("%s: Player turn, creature id: %d", __func__, creature_id);
  m_world->creatureTurn(creature_id, direction);
}

void GameEngine::say(CreatureId creature_id,
                     int type,
                     const std::string& message,
                     const std::string& receiver,
                     int channel_id)
{
  // TODO(simon): care about these
  (void)type;
  (void)receiver;
  (void)channel_id;

  auto& player_data = getPlayerData(creature_id);

  LOG_DEBUG("%s: creature_id: %d, message: %s", __func__, creature_id, message.c_str());

  // Check if message is a command
  if (!message.empty() && message[0] == '/')
  {
    // Extract everything after '/'
    auto full_command = message.substr(1, std::string::npos);

    // Check if there is arguments
    auto space = full_command.find_first_of(' ');
    std::string command;
    std::string option;
    if (space == std::string::npos)
    {
      command = full_command;
      option = "";
    }
    else
    {
      command = full_command.substr(0, space);
      option = full_command.substr(space + 1);
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
        position = m_world->getCreaturePosition(creature_id);
      }
      else if (command == "debugf")
      {
        // Show debug information on tile in front of player
        position = m_world->getCreaturePosition(creature_id).addDirection(player_data.player.getDirection());
      }

      const auto* tile = static_cast<const World*>(m_world.get())->getTile(position);

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

      player_data.player_ctrl->sendTextMessage(0x13, oss.str());
    }
    else if (command == "put")
    {
      // TODO(simon): replace itemManager->createItem/getItem with world->createItem/getItem

//      std::istringstream iss(option);
//      ItemTypeId item_typeId = 0;
//      iss >> item_typeId;
//
//      const auto itemId = m_item_manager->createItem(item_typeId);
//
//      if (itemId == 0)  // TODO(simon): see item_manager TODO
//      {
//        player_data.player_ctrl->sendTextMessage(0x13, "Invalid itemId");
//      }
//      else
//      {
//        auto* item = m_item_manager->getItem(itemId);
//        const auto position = m_world->getCreaturePosition(creature_id).addDirection(player_data.player.getDirection());
//        m_world->addItem(item, position);
//      }
    }
    else
    {
      player_data.player_ctrl->sendTextMessage(0x13, "Invalid command");
    }
  }
  else
  {
    m_world->creatureSay(creature_id, message);
  }
}

void GameEngine::moveItem(CreatureId creature_id,
                          const ItemPosition& from_position,
                          const GamePosition& to_position,
                          int count)
{
  LOG_DEBUG("%s: creature_id: %d from_position: %s to_position: %s count: %d",
            __func__,
            creature_id,
            from_position.toString().c_str(),
            to_position.toString().c_str(),
            count);

  auto& player_data = getPlayerData(creature_id);

  if (from_position.getItemTypeId() == 0x63)
  {
    // Move Creature
    player_data.player_ctrl->sendTextMessage(0x13, "Not yet implemented.");
    return;
  }

  // Verify that the Item is at from_position and can be moved
  if (!getItem(creature_id, from_position))
  {
    LOG_ERROR("%s: could not find Item at from_position: %s", __func__, from_position.toString().c_str());
    return;
  }
  auto* item = getItem(creature_id, from_position);

  // TODO(simon): verify that Item is movable

  // TODO(simon): check if to_position points to a container item, and in that case change to_position
  //              to point inside that container (if applicable)

  // Verify that the Item can be added to to_position
  if (!canAddItem(creature_id, to_position, *item, count))
  {
    // TODO(simon): proper error message to player
    LOG_ERROR("%s: cannot add Item to to_position: %s", __func__, to_position.toString().c_str());
    return;
  }

  // Remove Item from from_position
  removeItem(creature_id, from_position, count);

  // Add Item to to_position
  addItem(creature_id, to_position, *item, count);
}

void GameEngine::useItem(CreatureId creature_id, const ItemPosition& position, int new_container_id)
{
  LOG_DEBUG("%s: creature_id: %d position: %s new_container_id: %d",
            __func__,
            creature_id,
            position.toString().c_str(),
            new_container_id);

  auto& player_data = getPlayerData(creature_id);

  auto* item = getItem(creature_id, position);
  if (!item)
  {
    LOG_ERROR("%s: could not find Item", __func__);
    return;
  }

  // TODO(simon): verify that player is close enough, if item is in world

  if (item->getItemType().is_container)
  {
    m_container_manager->useContainer(player_data.player_ctrl, *item, position.getGamePosition(), new_container_id);
  }
}

void GameEngine::lookAt(CreatureId creature_id, const ItemPosition& position)
{
  LOG_DEBUG("%s: creature_id: %d position: %s",
            __func__,
            creature_id,
            position.toString().c_str());

  auto& player_data = getPlayerData(creature_id);

  auto* item = getItem(creature_id, position);
  if (!item)
  {
    LOG_ERROR("%s: could not find Item", __func__);
    return;
  }

  // TODO(simon): verify that player is close enough, if item is in world

  const auto& item_type = item->getItemType();

  std::ostringstream ss;

  if (!item_type.name.empty())
  {
    if (item_type.is_stackable && item->getCount() > 1)
    {
      ss << "You see " << item->getCount() << " " << item_type.name << "s.";
    }
    else
    {
      ss << "You see a " << item_type.name << ".";
    }
  }
  else
  {
    ss << "You see an item with ItemTypeId: " << item->getItemTypeId() << ".";
  }

  // TODO(simon): only if standing next to the item
  if (item_type.weight != 0)
  {
    ss << "\nIt weights " << item_type.weight << " oz.";
  }

  if (!item_type.descr.empty())
  {
    ss << "\n" << item_type.descr;
  }

  player_data.player_ctrl->sendTextMessage(0x13, ss.str());
}

void GameEngine::closeContainer(CreatureId creature_id, ItemUniqueId item_unique_id)
{
  LOG_DEBUG("%s: creature_id: %d item_unique_id: %d", __func__, creature_id, item_unique_id);
  m_container_manager->closeContainer(getPlayerData(creature_id).player_ctrl, item_unique_id);
}

void GameEngine::openParentContainer(CreatureId creature_id, ItemUniqueId item_unique_id, int new_container_id)
{
  LOG_DEBUG("%s: creature_id: %d item_unique_id: %d", __func__, creature_id, item_unique_id);
  m_container_manager->openParentContainer(getPlayerData(creature_id).player_ctrl,
                                           item_unique_id,
                                           new_container_id);
}

const Item* GameEngine::getItem(CreatureId creature_id, const ItemPosition& position)
{
  // TODO(simon): verify ItemId
  auto& player_data = getPlayerData(creature_id);

  const auto& game_position = position.getGamePosition();
  if (game_position.isPosition())
  {
    const auto* tile = static_cast<const World*>(m_world.get())->getTile(game_position.getPosition());
    if (tile)
    {
      return tile->getThing(position.getStackPosition())->item;
    }
  }
  else if (game_position.isInventory())
  {
    return player_data.player.getEquipment().getItem(game_position.getInventorySlot());
  }
  else if (game_position.isContainer())
  {
    return m_container_manager->getItem(game_position.getItemUniqueId(),
                                      game_position.getContainerSlot());
  }

  LOG_ERROR("%s: GamePosition: %s, is invalid", __func__, game_position.toString().c_str());
  return nullptr;
}

bool GameEngine::canAddItem(CreatureId creature_id, const GamePosition& position, const Item& item, int count)
{
  auto& player_data = getPlayerData(creature_id);

  // TODO(simon): count
  (void)count;

  if (position.isPosition())
  {
    return m_world->canAddItem(item, position.getPosition());
  }

  if (position.isInventory())
  {
    return player_data.player.getEquipment().canAddItem(item, position.getInventorySlot());
  }

  if (position.isContainer())
  {
    // TODO(simon): check capacity of Player if root Container is in Player inventory
    return m_container_manager->canAddItem(position.getItemUniqueId(),
                                         position.getContainerSlot(),
                                         item);
  }

  LOG_ERROR("%s: invalid position: %s", __func__, position.toString().c_str());
  return false;
}

void GameEngine::removeItem(CreatureId creature_id, const ItemPosition& position, int count)
{
  auto& player_data = getPlayerData(creature_id);

  // TODO(simon): count
  (void)count;

  // TODO(simon): verify itemId? verify success of removal?

  if (position.getGamePosition().isPosition())
  {
    m_world->removeItem(position.getItemTypeId(),
                       1,
                       position.getGamePosition().getPosition(),
                       position.getStackPosition());
  }
  else if (position.getGamePosition().isInventory())
  {
    player_data.player.getEquipment().removeItem(position.getItemTypeId(),
                                                position.getGamePosition().getInventorySlot());
    player_data.player_ctrl->onEquipmentUpdated(player_data.player, position.getGamePosition().getInventorySlot());
  }
  else if (position.getGamePosition().isContainer())
  {
    m_container_manager->removeItem(position.getGamePosition().getItemUniqueId(),
                                  position.getGamePosition().getContainerSlot());
  }
}

void GameEngine::addItem(CreatureId creature_id, const GamePosition& position, const Item& item, int count)
{
  auto& player_data = getPlayerData(creature_id);

  // TODO(simon): count
  (void)count;

  // TODO(simon): verify success? But how do we handle failure, rollback?
  if (position.isPosition())
  {
    m_world->addItem(item, position.getPosition());
  }
  else if (position.isInventory())
  {
    player_data.player.getEquipment().addItem(item, position.getInventorySlot());
    getPlayerData(creature_id).player_ctrl->onEquipmentUpdated(player_data.player, position.getInventorySlot());
  }
  else if (position.isContainer())
  {
    // Note: We cannot assume that the item is added to the container referenced in position
    //       If the containerSlot points to a container-item than the item will be added
    //       to that inner container
    m_container_manager->addItem(position.getItemUniqueId(),
                               position.getContainerSlot(),
                               item);
  }

  // Handle case where a container is moved to a non-container
  if (item.getItemType().is_container && !position.isContainer())
  {
    m_container_manager->updateRootPosition(item.getItemUniqueId(), position);
  }
}
