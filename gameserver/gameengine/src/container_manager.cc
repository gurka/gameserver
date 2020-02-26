/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Simon Sandström
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

#include "container_manager.h"

#include <algorithm>
#include <functional>
#include <iterator>

#include "item.h"
#include "logger.h"
#include "player_ctrl.h"

namespace gameengine
{

void ContainerManager::playerDespawn(const PlayerCtrl* player_ctrl)
{
  for (auto item_unique_id : player_ctrl->getContainerIds())
  {
    if (item_unique_id != common::Item::INVALID_UNIQUE_ID)
    {
      removeRelatedPlayer(player_ctrl, item_unique_id);
    }
  }
}

const Container* ContainerManager::getContainer(common::ItemUniqueId item_unique_id) const
{
  if (item_unique_id == common::Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: invalid item_unique_id: %d", __func__, item_unique_id);
    return nullptr;
  }

  // Verify that the container exists
  if (m_containers.count(item_unique_id) == 0)
  {
    LOG_ERROR("%s: no container found with item_unique_id: %d", __func__, item_unique_id);
    return nullptr;
  }

  return &m_containers.at(item_unique_id);
}

Container* ContainerManager::getContainer(common::ItemUniqueId item_unique_id)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* container = static_cast<const ContainerManager*>(this)->getContainer(item_unique_id);
  return const_cast<Container*>(container);
}

const common::Item* ContainerManager::getItem(common::ItemUniqueId item_unique_id, int container_slot) const
{
  const auto* container = getContainer(item_unique_id);
  if (!container)
  {
    // getContainer logs on error
    return nullptr;
  }

  if (container_slot < 0 || container_slot >= static_cast<int>(container->items.size()))
  {
    LOG_ERROR("%s: invalid container_slot: %d for item_unique_id: %d", __func__, container_slot, item_unique_id);
    return nullptr;
  }

  return container->items[container_slot];
}

void ContainerManager::useContainer(PlayerCtrl* player_ctrl,
                                    const common::Item& item,
                                    const common::GamePosition& game_position,
                                    int new_container_id)
{
  if (!item.getItemType().is_container)
  {
    LOG_ERROR("%s: item with itemTypeId %d is not a container", __func__, item.getItemTypeId());
    return;
  }

  if (m_containers.count(item.getItemUniqueId()) == 0)
  {
    // Create the container
    createContainer(&item, game_position);
  }

  if (player_ctrl->hasContainerOpen(item.getItemUniqueId()))
  {
    // Do not close the Container here, the client will ack this by sending closeContainer
    player_ctrl->onCloseContainer(item.getItemUniqueId(), false);
  }
  else
  {
    openContainer(player_ctrl, item.getItemUniqueId(), new_container_id);
  }
}

void ContainerManager::closeContainer(PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id)
{
  LOG_DEBUG("%s: playerId: %d, item_unique_id: %d", __func__, player_ctrl->getPlayerId(), item_unique_id);

  // Remove player from related players
  removeRelatedPlayer(player_ctrl, item_unique_id);

  // Send onCloseContainer
  player_ctrl->onCloseContainer(item_unique_id, true);
}

void ContainerManager::openParentContainer(PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id, int new_container_id)
{
  auto* current_container = getContainer(item_unique_id);
  if (!current_container)
  {
    LOG_ERROR("%s: no container found with item_unique_id: %d", __func__, item_unique_id);
    return;
  }

  // Remove player from current container
  removeRelatedPlayer(player_ctrl, item_unique_id);

  // Open parent container
  openContainer(player_ctrl, current_container->parent_item_unique_id, new_container_id);
}

bool ContainerManager::canAddItem(common::ItemUniqueId item_unique_id,
                                  int container_slot,
                                  const common::Item& item)
{
  LOG_DEBUG("%s: item_unique_id: %d, container_slot: %d, itemTypeId: %d",
            __func__,
            item_unique_id,
            container_slot,
            item.getItemTypeId());

  // TODO(simon): GameEngine is responsible to check if the player has this container open

  auto* container = getContainer(item_unique_id);
  if (!container)
  {
    // getContainer logs error
    return false;
  }

  // If container_slot points to another contianer, create it if needed and reset container pointer
  container = getInnerContainer(container, container_slot);

  // Just make sure that there is room for the item
  // GameEngine is responsible for checking the weight and player capacity
  const auto container_max_items = container->item->getItemType().maxitems;
  LOG_DEBUG("%s: container->items.size(): %d, container_max_items: %d",
            __func__,
            container->items.size(),
            container_max_items);
  return static_cast<int>(container->items.size()) < container_max_items;
}

void ContainerManager::removeItem(common::ItemUniqueId item_unique_id, int container_slot)
{
  LOG_DEBUG("%s: item_unique_id: %d, container_slot: %d",
            __func__,
            item_unique_id,
            container_slot);

  auto* container = getContainer(item_unique_id);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  // Make sure that the container_slot is valid
  if (container_slot < 0 || container_slot >= static_cast<int>(container->items.size()))
  {
    LOG_ERROR("%s: invalid container_slot: %d, container->items.size(): %d",
              __func__,
              container_slot,
              static_cast<int>(container->items.size()));
    return;
  }

  const auto* item = container->items[container_slot];

  // Remove the item
  container->items.erase(container->items.begin() + container_slot);

  // Check if the removed item is a container
  if (item->getItemType().is_container)
  {
    // Update parent_item_unique_id and root_game_position if there is a container created for the item
    if (m_containers.count(item->getItemUniqueId()) == 1)
    {
      m_containers[item->getItemUniqueId()].parent_item_unique_id = common::Item::INVALID_UNIQUE_ID;
      m_containers[item->getItemUniqueId()].root_game_position = common::GamePosition();
    }
  }

  // Inform players that have this contianer open about the change
  for (auto& player_ctrl : container->related_players)
  {
    player_ctrl->onContainerRemoveItem(item_unique_id, container_slot);
  }
}

void ContainerManager::addItem(common::ItemUniqueId item_unique_id, int container_slot, const common::Item& item)
{
  LOG_DEBUG("%s: item_unique_id: %d, container_slot: %d, itemTypeId: %d",
            __func__,
            item_unique_id,
            container_slot,
            item.getItemTypeId());

  auto* container = getContainer(item_unique_id);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  // TODO(simon): Check if the container is full

  // If container_slot points to another contianer, create it if needed and reset container pointer
  container = getInnerContainer(container, container_slot);

  // Add the item at the front
  container->items.insert(container->items.begin(), &item);

  // Check if the new item is a container
  if (item.getItemType().is_container)
  {
    // Update parent_item_unique_id and root_game_position if there is a container created for this item
    // Otherwise it will be done in createContainer when the container is opened
    if (m_containers.count(item.getItemUniqueId()) == 1)
    {
      m_containers[item.getItemUniqueId()].parent_item_unique_id = container->item->getItemUniqueId();
      m_containers[item.getItemUniqueId()].root_game_position = container->root_game_position;
    }
  }

  // Inform players that have this container open about the change
  for (auto& player_ctrl : container->related_players)
  {
    player_ctrl->onContainerAddItem(item_unique_id, item);
  }
}

void ContainerManager::updateRootPosition(common::ItemUniqueId item_unique_id, const common::GamePosition& game_position)
{
  // Note: this function should only be used when a container is moved to a non-container
  //       (e.g. inventory or world position)

  if (m_containers.count(item_unique_id) == 0)
  {
    // This is OK and can occur if moving a (not created/opened) container from/to a
    // non-container position
    return;
  }

  auto* container = getContainer(item_unique_id);

  if (game_position.isContainer())
  {
    LOG_ERROR("%s: use addItem for moving a container into another container", __func__);
    return;
  }

  LOG_DEBUG("%s: item_unique_id: %d game_position: %s", __func__, item_unique_id, game_position.toString().c_str());

  // Update container's and all inner container's root_game_position
  const auto update_containers = [this, &game_position](Container* container, auto& update_m_containersref) -> void
  {
    container->root_game_position = game_position;
    for (auto* item : container->items)
    {
      if (item->getItemType().is_container)
      {
        // Don't update container's which hasn't been created yet,
        // and don't use getContainer to avoid ERROR log
        if (m_containers.count(item->getItemUniqueId()) == 1)
        {
          update_m_containersref(&m_containers[item->getItemUniqueId()], update_m_containersref);
        }
      }
    }
  };
  update_containers(container, update_containers);
}

Container* ContainerManager::getInnerContainer(Container* container, int container_slot)
{
  if (container_slot >= 0 &&
      container_slot < static_cast<int>(container->items.size()) &&
      container->items[container_slot]->getItemType().is_container)
  {
    // We might need to make a new Container object for the inner container
    if (m_containers.count(container->items[container_slot]->getItemUniqueId()) == 0)
    {
      createContainer(container->items[container_slot],
                      common::GamePosition(container->item->getItemUniqueId(), container_slot));
    }

    // Change the container pointer to the inner container
    return &m_containers.at(container->items[container_slot]->getItemUniqueId());
  }

  return container;
}

void ContainerManager::createContainer(const common::Item* item, const common::GamePosition& game_position)
{
  if (m_containers.count(item->getItemUniqueId()) == 1)
  {
    LOG_ERROR("%s: container is already created for item_unique_id: %d", __func__, item->getItemUniqueId());
    return;
  }

  auto& container = m_containers[item->getItemUniqueId()];
  container.weight = 0;
  container.item = item;
  if (game_position.isPosition() ||
      game_position.isInventory())
  {
    container.parent_item_unique_id = common::Item::INVALID_UNIQUE_ID;
    container.root_game_position = game_position;
  }
  else  // isContainer
  {
    container.parent_item_unique_id = game_position.getItemUniqueId();
    container.root_game_position = m_containers.at(container.parent_item_unique_id).root_game_position;
  }
  container.items = {};
  container.related_players = {};

  LOG_DEBUG("%s: created new Container with item_unique_id %d, parent_item_unique_id: %d, rootItemPosition: %s",
            __func__,
            item->getItemUniqueId(),
            container.parent_item_unique_id,
            container.root_game_position.toString().c_str());
}

void ContainerManager::openContainer(PlayerCtrl* player_ctrl,
                                     common::ItemUniqueId item_unique_id,
                                     int new_container_id)
{
  LOG_DEBUG("%s: playerId: %d, item_unique_id: %d, new_container_id: %d",
            __func__,
            player_ctrl->getPlayerId(),
            item_unique_id,
            new_container_id);

  // Check if player already has a container open with this id
  if (player_ctrl->getContainerIds()[new_container_id] != common::Item::INVALID_UNIQUE_ID)
  {
    // Then remove player from that container's related_players
    removeRelatedPlayer(player_ctrl, player_ctrl->getContainerIds()[new_container_id]);
  }

  auto& container = m_containers.at(item_unique_id);

  // Add player to related players
  addRelatedPlayer(player_ctrl, item_unique_id);

  // Send onOpenContainer
  player_ctrl->onOpenContainer(new_container_id, container, *container.item);
}

void ContainerManager::addRelatedPlayer(PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id)
{
  auto* container = getContainer(item_unique_id);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  container->related_players.emplace_back(player_ctrl);
}

void ContainerManager::removeRelatedPlayer(const PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id)
{
  auto* container = getContainer(item_unique_id);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  auto it = std::find(container->related_players.begin(), container->related_players.end(), player_ctrl);
  if (it == container->related_players.end())
  {
    LOG_ERROR("%s: could not find RelatedPlayer with playerId: %d in item_unique_id: %d",
              __func__,
              player_ctrl->getPlayerId(),
              item_unique_id);
    return;
  }

  container->related_players.erase(it);
}

}  // namespace gameengine
