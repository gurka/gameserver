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

#include "container_manager.h"

#include <algorithm>
#include <functional>
#include <iterator>

#include "item.h"
#include "logger.h"
#include "player_ctrl.h"

void ContainerManager::playerDespawn(const PlayerCtrl* playerCtrl)
{
  for (auto itemUniqueId : playerCtrl->getContainerIds())
  {
    if (itemUniqueId != Item::INVALID_UNIQUE_ID)
    {
      removeRelatedPlayer(playerCtrl, itemUniqueId);
    }
  }
}

const Container* ContainerManager::getContainer(ItemUniqueId itemUniqueId) const
{
  if (itemUniqueId == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: invalid itemUniqueId: %d", __func__, itemUniqueId);
    return nullptr;
  }

  // Verify that the container exists
  if (containers_.count(itemUniqueId) == 0)
  {
    LOG_ERROR("%s: no container found with itemUniqueId: %d", __func__, itemUniqueId);
    return nullptr;
  }

  return &containers_.at(itemUniqueId);
}

Container* ContainerManager::getContainer(ItemUniqueId itemUniqueId)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* container = static_cast<const ContainerManager*>(this)->getContainer(itemUniqueId);
  return const_cast<Container*>(container);
}

const Item* ContainerManager::getItem(ItemUniqueId itemUniqueId, int containerSlot) const
{
  const auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs on error
    return nullptr;
  }

  if (containerSlot < 0 || containerSlot >= static_cast<int>(container->items.size()))
  {
    LOG_ERROR("%s: invalid containerSlot: %d for itemUniqueId: %d", __func__, containerSlot, itemUniqueId);
    return nullptr;
  }

  return container->items[containerSlot];
}

Item* ContainerManager::getItem(ItemUniqueId itemUniqueId, int containerSlot)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* item = static_cast<const ContainerManager*>(this)->getItem(itemUniqueId, containerSlot);
  return const_cast<Item*>(item);
}

void ContainerManager::useContainer(PlayerCtrl* playerCtrl,
                                    const Item& item,
                                    const GamePosition& gamePosition,
                                    int newContainerId)
{
  if (!item.getItemType().isContainer)
  {
    LOG_ERROR("%s: item with itemTypeId %d is not a container", __func__, item.getItemTypeId());
    return;
  }

  if (containers_.count(item.getItemUniqueId()) == 0)
  {
    // Create the container
    createContainer(&item, gamePosition);
  }

  if (playerCtrl->hasContainerOpen(item.getItemUniqueId()))
  {
    // Do not close the Container here, the client will ack this by sending closeContainer
    playerCtrl->onCloseContainer(item.getItemUniqueId(), false);
  }
  else
  {
    openContainer(playerCtrl, item.getItemUniqueId(), newContainerId);
  }
}

void ContainerManager::closeContainer(PlayerCtrl* playerCtrl, ItemUniqueId itemUniqueId)
{
  LOG_DEBUG("%s: playerId: %d, itemUniqueId: %d", __func__, playerCtrl->getPlayerId(), itemUniqueId);

  // Remove player from related players
  removeRelatedPlayer(playerCtrl, itemUniqueId);

  // Send onCloseContainer
  playerCtrl->onCloseContainer(itemUniqueId, true);
}

void ContainerManager::openParentContainer(PlayerCtrl* playerCtrl, ItemUniqueId itemUniqueId, int newContainerId)
{
  auto* currentContainer = getContainer(itemUniqueId);
  if (!currentContainer)
  {
    LOG_ERROR("%s: no container found with itemUniqueId: %d", __func__, itemUniqueId);
    return;
  }

  // Remove player from current container
  removeRelatedPlayer(playerCtrl, itemUniqueId);

  // Open parent container
  openContainer(playerCtrl, currentContainer->parentItemUniqueId, newContainerId);
}

bool ContainerManager::canAddItem(ItemUniqueId itemUniqueId,
                                  int containerSlot,
                                  const Item& item)
{
  LOG_DEBUG("%s: itemUniqueId: %d, containerSlot: %d, itemTypeId: %d",
            __func__,
            itemUniqueId,
            containerSlot,
            item.getItemTypeId());

  // TODO(simon): GameEngine is responsible to check if the player has this container open

  auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs error
    return false;
  }

  // If containerSlot points to another contianer, create it if needed and reset container pointer
  container = getInnerContainer(container, containerSlot);

  // Just make sure that there is room for the item
  // GameEngine is responsible for checking the weight and player capacity
  const auto containerItemMaxItems = container->item->getItemType().maxitems;
  LOG_DEBUG("%s: container->items.size(): %d, containerItemMaxItems: %d",
            __func__,
            container->items.size(),
            containerItemMaxItems);
  return static_cast<int>(container->items.size()) < containerItemMaxItems;
}

void ContainerManager::removeItem(ItemUniqueId itemUniqueId, int containerSlot)
{
  LOG_DEBUG("%s: itemUniqueId: %d, containerSlot: %d",
            __func__,
            itemUniqueId,
            containerSlot);

  auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  // Make sure that the containerSlot is valid
  if (containerSlot < 0 || containerSlot >= static_cast<int>(container->items.size()))
  {
    LOG_ERROR("%s: invalid containerSlot: %d, container->items.size(): %d",
              __func__,
              containerSlot,
              static_cast<int>(container->items.size()));
    return;
  }

  const auto* item = container->items[containerSlot];

  // Remove the item
  container->items.erase(container->items.begin() + containerSlot);

  // Check if the removed item is a container
  if (item->getItemType().isContainer)
  {
    // Update parentItemUniqueId and rootGamePosition if there is a container created for the item
    if (containers_.count(item->getItemUniqueId()) == 1)
    {
      containers_[item->getItemUniqueId()].parentItemUniqueId = Item::INVALID_UNIQUE_ID;
      containers_[item->getItemUniqueId()].rootGamePosition = GamePosition();
    }
  }

  // Inform players that have this contianer open about the change
  for (auto& playerCtrl : container->relatedPlayers)
  {
    playerCtrl->onContainerRemoveItem(itemUniqueId, containerSlot);
  }
}

void ContainerManager::addItem(ItemUniqueId itemUniqueId, int containerSlot, Item* item)
{
  LOG_DEBUG("%s: itemUniqueId: %d, containerSlot: %d, itemTypeId: %d",
            __func__,
            itemUniqueId,
            containerSlot,
            item->getItemTypeId());

  auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  // TODO(simon): Check if the container is full

  // If containerSlot points to another contianer, create it if needed and reset container pointer
  container = getInnerContainer(container, containerSlot);

  // Add the item at the front
  container->items.insert(container->items.begin(), item);

  // Check if the new item is a container
  if (item->getItemType().isContainer)
  {
    // Update parentItemUniqueId and rootGamePosition if there is a container created for this item
    // Otherwise it will be done in createContainer when the container is opened
    if (containers_.count(item->getItemUniqueId()) == 1)
    {
      containers_[item->getItemUniqueId()].parentItemUniqueId = container->item->getItemUniqueId();
      containers_[item->getItemUniqueId()].rootGamePosition = container->rootGamePosition;
    }
  }

  // Inform players that have this container open about the change
  for (auto& playerCtrl : container->relatedPlayers)
  {
    playerCtrl->onContainerAddItem(itemUniqueId, *item);
  }
}

void ContainerManager::updateRootPosition(ItemUniqueId itemUniqueId, const GamePosition& gamePosition)
{
  // Note: this function should only be used when a container is moved to a non-container
  //       (e.g. inventory or world position)

  auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  if (gamePosition.isContainer())
  {
    LOG_ERROR("%s: use addItem for moving a container into another container", __func__);
    return;
  }

  LOG_DEBUG("%s: itemUniqueId: %d gamePosition: %s", __func__, itemUniqueId, gamePosition.toString().c_str());

  // Update container's and all inner container's rootGamePosition
  const auto updateContainers = [this, &gamePosition](Container* container, auto& updateContainersRef) -> void
  {
    container->rootGamePosition = gamePosition;
    for (auto* item : container->items)
    {
      if (item->getItemType().isContainer)
      {
        // Don't update container's which hasn't been created yet,
        // and don't use getContainer to avoid ERROR log
        if (containers_.count(item->getItemUniqueId()) == 1)
        {
          updateContainersRef(&containers_[item->getItemUniqueId()], updateContainersRef);
        }
      }
    }
  };
  updateContainers(container, updateContainers);
}

Container* ContainerManager::getInnerContainer(Container* container, int containerSlot)
{
  if (containerSlot >= 0 &&
      containerSlot < static_cast<int>(container->items.size()) &&
      container->items[containerSlot]->getItemType().isContainer)
  {
    // We might need to make a new Container object for the inner container
    if (containers_.count(container->items[containerSlot]->getItemUniqueId()) == 0)
    {
      createContainer(container->items[containerSlot],
                      GamePosition(container->item->getItemUniqueId(), containerSlot));
    }

    // Change the container pointer to the inner container
    return &containers_.at(container->items[containerSlot]->getItemUniqueId());
  }

  return container;
}

void ContainerManager::createContainer(const Item* item, const GamePosition& gamePosition)
{
  if (containers_.count(item->getItemUniqueId()))
  {
    LOG_ERROR("%s: container is already created for itemUniqueId: %d", __func__, item->getItemUniqueId());
    return;
  }

  auto& container = containers_[item->getItemUniqueId()];
  container.weight = 0;
  container.item = item;
  if (gamePosition.isPosition() ||
      gamePosition.isInventory())
  {
    container.parentItemUniqueId = Item::INVALID_UNIQUE_ID;
    container.rootGamePosition = gamePosition;
  }
  else  // isContainer
  {
    container.parentItemUniqueId = gamePosition.getItemUniqueId();
    container.rootGamePosition = containers_.at(container.parentItemUniqueId).rootGamePosition;
  }
  container.items = {};
  container.relatedPlayers = {};

  LOG_DEBUG("%s: created new Container with itemUniqueId %d, parentItemUniqueId: %d, rootItemPosition: %s",
            __func__,
            item->getItemUniqueId(),
            container.parentItemUniqueId,
            container.rootGamePosition.toString().c_str());
}

void ContainerManager::openContainer(PlayerCtrl* playerCtrl,
                                     ItemUniqueId itemUniqueId,
                                     int newContainerId)
{
  LOG_DEBUG("%s: playerId: %d, itemUniqueId: %d, newContainerId: %d",
            __func__,
            playerCtrl->getPlayerId(),
            itemUniqueId,
            newContainerId);

  // Check if player already has a container open with this id
  if (playerCtrl->getContainerIds()[newContainerId] != Item::INVALID_UNIQUE_ID)
  {
    // Then remove player from that container's relatedPlayers
    removeRelatedPlayer(playerCtrl, playerCtrl->getContainerIds()[newContainerId]);
  }

  auto& container = containers_.at(itemUniqueId);

  // Add player to related players
  addRelatedPlayer(playerCtrl, itemUniqueId);

  // Send onOpenContainer
  playerCtrl->onOpenContainer(newContainerId, container, *container.item);
}

void ContainerManager::addRelatedPlayer(PlayerCtrl* playerCtrl, ItemUniqueId itemUniqueId)
{
  auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  container->relatedPlayers.emplace_back(playerCtrl);
}

void ContainerManager::removeRelatedPlayer(const PlayerCtrl* playerCtrl, ItemUniqueId itemUniqueId)
{
  auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  auto it = std::find(container->relatedPlayers.begin(), container->relatedPlayers.end(), playerCtrl);
  if (it == container->relatedPlayers.end())
  {
    LOG_ERROR("%s: could not find RelatedPlayer with playerId: %d in itemUniqueId: %d",
              __func__,
              playerCtrl->getPlayerId(),
              itemUniqueId);
    return;
  }

  container->relatedPlayers.erase(it);
}
