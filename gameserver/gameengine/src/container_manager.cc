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
#include <iterator>

#include "item.h"
#include "logger.h"
#include "player_ctrl.h"

void ContainerManager::playerSpawn(const PlayerCtrl* playerCtrl)
{
  auto& clientContainerIds = clientContainerIds_[playerCtrl->getPlayerId()];
  clientContainerIds.fill(Item::INVALID_UNIQUE_ID);
}

void ContainerManager::playerDespawn(const PlayerCtrl* playerCtrl)
{
  const auto playerId = playerCtrl->getPlayerId();
  if (clientContainerIds_.count(playerId) == 1)
  {
    auto& clientContainerIds = clientContainerIds_[playerId];
    for (auto i = 0u; i < clientContainerIds.size(); i++)
    {
      const auto containerId = clientContainerIds[i];
      if (containerId == Item::INVALID_UNIQUE_ID)
      {
        continue;
      }
      removeRelatedPlayer(playerCtrl, i);
    }

    clientContainerIds_.erase(playerId);
  }
}

ItemUniqueId ContainerManager::getItemUniqueId(const PlayerCtrl* playerCtrl, int containerId) const
{
  return clientContainerIds_.at(playerCtrl->getPlayerId())[containerId];
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
                                    const ItemPosition& itemPosition,
                                    int newClientContainerId)
{
  if (!item.getItemType().isContainer)
  {
    LOG_ERROR("%s: item with itemTypeId %d is not a container", __func__, item.getItemTypeId());
    return;
  }

  if (containers_.count(item.getItemUniqueId()) == 0)
  {
    // Create the container
    createContainer(&item, itemPosition);
  }
  else
  {
    // TODO: Can we validate that the container is where it's supposed to be?
  }

  // Check if Player has this Container open
  const auto clientContainerId = getClientContainerId(playerCtrl->getPlayerId(), item.getItemUniqueId());
  if (clientContainerId == -1)
  {
    openContainer(playerCtrl, item.getItemUniqueId(), newClientContainerId);
  }
  else
  {
    // Do not close the Container here, the client will ack this by sending closeContainer
    playerCtrl->onCloseContainer(clientContainerId);
  }
}

void ContainerManager::closeContainer(PlayerCtrl* playerCtrl, int clientContainerId)
{
  const auto itemUniqueId = getItemUniqueId(playerCtrl, clientContainerId);
  if (itemUniqueId == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: must be called with clientContainerId", __func__);
    return;
  }

  closeContainer(playerCtrl, itemUniqueId, clientContainerId);
}

void ContainerManager::openParentContainer(PlayerCtrl* playerCtrl, int clientContainerId)
{
  const auto itemUniqueId = getItemUniqueId(playerCtrl, clientContainerId);
  if (itemUniqueId == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: must be called with clientContainerId", __func__);
    return;
  }

  auto* currentContainer = getContainer(itemUniqueId);
  if (!currentContainer)
  {
    LOG_ERROR("%s: no container found with itemUniqueId: %d", __func__, itemUniqueId);
    return;
  }

  // Open parent container
  openContainer(playerCtrl, currentContainer->parentItemUniqueId, clientContainerId);
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

  // If the containerSlot is valid, then check if the item it points to is a container
  // TODO: duplicate code in ::addItem
  if (containerSlot >= 0 &&
      containerSlot < static_cast<int>(container->items.size()) &&
      container->items[containerSlot]->getItemType().isContainer)
  {
    // We might need to make a new Container object for the inner container
    if (containers_.count(container->items[containerSlot]->getItemUniqueId()) == 0)
    {
      createContainer(container->items[containerSlot],
                      ItemPosition(GamePosition(container->item->getItemUniqueId(),
                                                containerSlot),
                                   container->item->getItemTypeId()));
    }

    // Change the container pointer to the inner container
    container = &containers_.at(container->items[containerSlot]->getItemUniqueId());
  }

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

  // Remove the item
  container->items.erase(container->items.begin() + containerSlot);

  // Inform players that have this contianer open about the change
  for (auto& relatedPlayer : container->relatedPlayers)
  {
    relatedPlayer.playerCtrl->onContainerRemoveItem(relatedPlayer.clientContainerId, containerSlot);
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

  // If the containerSlot is valid, then check if the item it points to is a container
  // TODO: duplicate code in ::canAddItem
  if (containerSlot >= 0 &&
      containerSlot < static_cast<int>(container->items.size()) &&
      container->items[containerSlot]->getItemType().isContainer)
  {
    // We might need to make a new Container object for the inner container
    if (containers_.count(container->items[containerSlot]->getItemUniqueId()) == 0)
    {
      createContainer(container->items[containerSlot],
                      ItemPosition(GamePosition(container->item->getItemUniqueId(),
                                                containerSlot),
                                   container->item->getItemTypeId()));
    }

    // Change the container pointer to the inner container
    container = &containers_.at(container->items[containerSlot]->getItemUniqueId());
  }

  // Add the item at the front
  container->items.insert(container->items.begin(), item);

  // Inform players that have this container open about the change
  for (auto& relatedPlayer : container->relatedPlayers)
  {
    relatedPlayer.playerCtrl->onContainerAddItem(relatedPlayer.clientContainerId, *item);
  }
}

void ContainerManager::createContainer(const Item* item, const ItemPosition& itemPosition)
{
  if (containers_.count(item->getItemUniqueId()))
  {
    LOG_ERROR("%s: container is already created for itemUniqueId: %d", __func__, item->getItemUniqueId());
    return;
  }

  auto& container = containers_[item->getItemUniqueId()];
  container.weight = 0;
  container.item = item;
  if (itemPosition.getGamePosition().isPosition() ||
      itemPosition.getGamePosition().isInventory())
  {
    container.parentItemUniqueId = Item::INVALID_UNIQUE_ID;
    container.rootItemPosition = itemPosition;
  }
  else  // isContainer
  {
    const auto& parentContainer = containers_.at(itemPosition.getGamePosition().getItemUniqueId());
    container.parentItemUniqueId = itemPosition.getGamePosition().getItemUniqueId();
    container.rootItemPosition = parentContainer.rootItemPosition;
  }
  container.items = {};
  container.relatedPlayers = {};

  LOG_DEBUG("%s: created new Container with itemUniqueId %d, parentItemUniqueId: %d, rootItemPosition: %s",
            __func__,
            item->getItemUniqueId(),
            container.parentItemUniqueId,
            container.rootItemPosition.toString().c_str());
}

void ContainerManager::openContainer(PlayerCtrl* playerCtrl,
                                     ItemUniqueId itemUniqueId,
                                     int clientContainerId)
{
  LOG_DEBUG("%s: playerId: %d, itemUniqueId: %d, clientContainerId: %d",
            __func__,
            playerCtrl->getPlayerId(),
            itemUniqueId,
            clientContainerId);

  // Check if there already is an open container at the given clientContainerId
  const auto currentItemUniqueId = getContainerId(playerCtrl->getPlayerId(), clientContainerId);
  if (currentItemUniqueId != Item::INVALID_UNIQUE_ID)
  {
    // Remove player from relatedPlayers
    removeRelatedPlayer(playerCtrl, clientContainerId);
  }

  auto& container = containers_.at(itemUniqueId);

  // Set clientContainerId
  setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, itemUniqueId);

  // Add player to related players
  addRelatedPlayer(playerCtrl, clientContainerId);

  // Send onOpenContainer
  playerCtrl->onOpenContainer(clientContainerId, container, *container.item);
}

void ContainerManager::closeContainer(PlayerCtrl* playerCtrl,
                                      ItemUniqueId itemUniqueId,
                                      int clientContainerId)
{
  LOG_DEBUG("%s: playerId: %d, itemUniqueId: %d, clientContainerId: %d",
            __func__,
            playerCtrl->getPlayerId(),
            itemUniqueId,
            clientContainerId);

  // Remove player from related players
  removeRelatedPlayer(playerCtrl, clientContainerId);

  // Unset clientContainerId
  setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, Item::INVALID_UNIQUE_ID);

  // Send onCloseContainer
  playerCtrl->onCloseContainer(clientContainerId);
}

bool ContainerManager::isClientContainerId(int containerId) const
{
  return containerId >= 0 && containerId < 64;
}

void ContainerManager::setClientContainerId(CreatureId playerId, int clientContainerId, int containerId)
{
  auto& clientContainerIds = clientContainerIds_.at(playerId);
  clientContainerIds[clientContainerId] = containerId;
}

int ContainerManager::getClientContainerId(CreatureId playerId, int containerId) const
{
  const auto& clientContainerIds = clientContainerIds_.at(playerId);
  const auto it = std::find(clientContainerIds.cbegin(),
                            clientContainerIds.cend(),
                            containerId);
  if (it != clientContainerIds.cend())
  {
    return std::distance(clientContainerIds.cbegin(), it);
  }
  else
  {
    return -1;  // TODO(simon): constant?
  }
}

ItemUniqueId ContainerManager::getContainerId(CreatureId playerId, int clientContainerId) const
{
  if (!isClientContainerId(clientContainerId))
  {
    LOG_ERROR("%s: invalid clientContainerId: %d", __func__, clientContainerId);
    return Item::INVALID_UNIQUE_ID;
  }

  const auto& clientContainerIds = clientContainerIds_.at(playerId);
  return clientContainerIds[clientContainerId];
}

void ContainerManager::addRelatedPlayer(PlayerCtrl* playerCtrl, int clientContainerId)
{
  auto* container = getContainer(getItemUniqueId(playerCtrl, clientContainerId));
  if (!container)
  {
    // getContainer logs error
    return;
  }

  container->relatedPlayers.emplace_back(playerCtrl, clientContainerId);
}

void ContainerManager::removeRelatedPlayer(const PlayerCtrl* playerCtrl, int clientContainerId)
{
  const auto itemUniqueId = getItemUniqueId(playerCtrl, clientContainerId);
  auto* container = getContainer(itemUniqueId);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  auto it = std::find_if(container->relatedPlayers.begin(),
                         container->relatedPlayers.end(),
                         [playerCtrl, clientContainerId](const auto& relatedPlayer)
  {
    return relatedPlayer.playerCtrl->getPlayerId() == playerCtrl->getPlayerId() &&
           relatedPlayer.clientContainerId == clientContainerId;
  });
  if (it == container->relatedPlayers.end())
  {
    LOG_ERROR("%s: could not find RelatedPlayer with playerId: %d, clientContainerId: %d in itemUniqueId: %d",
              __func__,
              playerCtrl->getPlayerId(),
              clientContainerId,
              itemUniqueId);
    return;
  }

  container->relatedPlayers.erase(it);
}
