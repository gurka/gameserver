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

constexpr int Container::INVALID_ID;

void ContainerManager::playerSpawn(const PlayerCtrl* playerCtrl)
{
  auto& clientContainerIds = clientContainerIds_[playerCtrl->getPlayerId()];
  clientContainerIds.fill(Container::INVALID_ID);
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
      if (containerId == Container::INVALID_ID)
      {
        continue;
      }

      auto& container = containers_.at(containerId);
      removeRelatedPlayer(&container, playerCtrl, i);
    }

    clientContainerIds_.erase(playerId);
  }
}

const Container* ContainerManager::getContainer(int containerId) const
{
  if (containerId == Container::INVALID_ID)
  {
    LOG_ERROR("%s: invalid containerId: %d", __func__, containerId);
    return nullptr;
  }

  if (isClientContainerId(containerId))
  {
    LOG_ERROR("%s: containerId: %d is a clientContainerId", __func__, containerId);
    return nullptr;
  }

  // Verify that the container exists
  if (containers_.count(containerId) == 0)
  {
    LOG_ERROR("%s: no container found with id: %d", __func__, containerId);
    return nullptr;
  }

  return &containers_.at(containerId);
}

Container* ContainerManager::getContainer(int containerId)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* container = static_cast<const ContainerManager*>(this)->getContainer(containerId);
  return const_cast<Container*>(container);
}

const Container* ContainerManager::getContainer(const PlayerCtrl* playerCtrl, int containerId) const
{
  if (containerId == Container::INVALID_ID)
  {
    LOG_ERROR("%s: invalid containerId: %d", __func__, containerId);
    return nullptr;
  }

  // Convert from clientContainerId to containerId if needed
  if (isClientContainerId(containerId))
  {
    containerId = getContainerId(playerCtrl->getPlayerId(), containerId);
  }

  // Verify that the container exists
  if (containers_.count(containerId) == 0)
  {
    LOG_ERROR("%s: no container found with id: %d", __func__, containerId);
    return nullptr;
  }

  return &containers_.at(containerId);
}

Container* ContainerManager::getContainer(const PlayerCtrl* playerCtrl, int containerId)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* container = static_cast<const ContainerManager*>(this)->getContainer(playerCtrl, containerId);
  return const_cast<Container*>(container);
}

Item* ContainerManager::getItem(const PlayerCtrl* playerCtrl, int containerId, int containerSlot)
{
  auto* container = getContainer(playerCtrl, containerId);
  if (!container)
  {
    // getContainer logs on error
    return nullptr;
  }

  if (containerSlot < 0 || containerSlot >= static_cast<int>(container->items.size()))
  {
    LOG_ERROR("%s: invalid containerSlot: %d for containerId: %d", __func__, containerSlot, container->id);
    return nullptr;
  }

  return &container->items[containerSlot];
}

int ContainerManager::createContainer(PlayerCtrl* playerCtrl, ItemTypeId itemTypeId, const ItemPosition& itemPosition)
{
  const auto containerId = nextContainerId_;
  nextContainerId_ += 1;

  auto& container = containers_[containerId];
  container.id = containerId;
  container.weight = 0;
  container.itemId = itemId;
  if (itemPosition.getGamePosition().isPosition() ||
      itemPosition.getGamePosition().isInventory())
  {
    container.parentContainerId = Container::INVALID_ID;
    container.rootItemPosition = itemPosition;
  }
  else  // isContainer
  {
    const auto* parentContainer = getContainer(playerCtrl, itemPosition.getGamePosition().getContainerId());
    container.parentContainerId = parentContainer->id;
    container.rootItemPosition = parentContainer->rootItemPosition;
  }
  container.items = {};
  container.relatedPlayers = {};

  LOG_DEBUG("%s: created new Container with id %d, parentContainerId: %d, rootItemPosition: %s",
            __func__,
            container.id,
            container.parentContainerId,
            container.rootItemPosition.toString().c_str());

  return containerId;
}

void ContainerManager::useContainer(PlayerCtrl* playerCtrl, const Item& item, int newClientContainerId)
{
  if (!item.isContainer())
  {
    LOG_ERROR("%s: item with id %d is not a container", __func__, item.getItemId());
    return;
  }

  if (containers_.count(item.getContainerId()) == 0)
  {
    LOG_ERROR("%s: item has invalid containerId (%d)", __func__, item.getContainerId());
    return;
  }

  auto& container = containers_[item.getContainerId()];

  // Check if Player has this Container open
  const auto clientContainerId = getClientContainerId(playerCtrl->getPlayerId(), container.id);
  if (clientContainerId == Container::INVALID_ID)
  {
    openContainer(playerCtrl, &container, newClientContainerId, item);
  }
  else
  {
    // Do not close the Container here, the client will ack this by sending closeContainer
    playerCtrl->onCloseContainer(clientContainerId);
  }
}

void ContainerManager::closeContainer(PlayerCtrl* playerCtrl, int clientContainerId)
{
  if (!isClientContainerId(clientContainerId))
  {
    LOG_ERROR("%s: must be called with clientContainerId", __func__);
    return;
  }

  auto* container = getContainer(playerCtrl, clientContainerId);
  if (!container)
  {
    LOG_ERROR("%s: could not find container with clientContainerId: %d", __func__, clientContainerId);
    return;
  }

  closeContainer(playerCtrl, container, clientContainerId);
}

void ContainerManager::openParentContainer(PlayerCtrl* playerCtrl, int clientContainerId)
{
  if (!isClientContainerId(clientContainerId))
  {
    LOG_ERROR("%s: must be called with clientContainerId", __func__);
    return;
  }

  auto* currentContainer = getContainer(playerCtrl, clientContainerId);
  if (!currentContainer)
  {
    LOG_ERROR("%s: no container found with id: %d", __func__, clientContainerId);
    return;
  }

  auto* parentContainer = getContainer(playerCtrl, currentContainer->parentContainerId);
  if (!parentContainer)
  {
    LOG_ERROR("%s: no container found with id: %d", __func__, currentContainer->parentContainerId);
    return;
  }

  // Open parent container
  const auto parentContainerItem = Item(parentContainer->itemId);
  openContainer(playerCtrl, parentContainer, clientContainerId, parentContainerItem);
}

bool ContainerManager::canAddItem(const PlayerCtrl* playerCtrl,
                                  int clientContainerId,
                                  int containerSlot,
                                  const Item& item) const
{
  LOG_DEBUG("%s: playerId: %d, clientContainerId: %d, containerSlot: %d, itemId: %d",
            __func__,
            playerCtrl->getPlayerId(),
            clientContainerId,
            containerSlot,
            item.getItemId());

  // Note: using clientContainerId implies that the player has this container
  //       open, which we other would have to check
  if (!isClientContainerId(clientContainerId))
  {
    LOG_ERROR("%s: must be called with clientContainerId", __func__);
    return false;
  }

  auto* container = getContainer(playerCtrl, clientContainerId);
  if (!container)
  {
    // getContainer logs error
    return false;
  }

  // If the containerSlot is valid, then check if the item it points to is a container
  if (containerSlot >= 0 &&
      containerSlot < static_cast<int>(container->items.size()) &&
      container->items[containerSlot].isContainer())
  {
    // We might need to make a new Container object for the inner container
    if (container->items[containerSlot].getContainerId() == Container::INVALID_ID)
    {
      LOG_ERROR("%s: create new Container for inner container NOT YET IMPLEMENTED", __func__);
      return false;
    }

    // Change the container pointer to the inner container
    container = &containers_.at(container->items[containerSlot].getContainerId());
  }

  // Just make sure that there is room for the item
  // GameEngine is responsible for checking the weight and player capacity
  const auto containerItem = Item(container->itemId);
  const auto containerItemMaxItems = containerItem.getAttribute<int>("maxitems");
  LOG_DEBUG("%s: container->items.size(): %d, containerItemMaxItems: %d",
            __func__,
            container->items.size(),
            containerItemMaxItems);
  return static_cast<int>(container->items.size()) < containerItemMaxItems;
}

void ContainerManager::removeItem(const PlayerCtrl* playerCtrl, int containerId, int containerSlot)
{
  LOG_DEBUG("%s: playerId: %d, containerId: %d, containerSlot: %d",
            __func__,
            playerCtrl->getPlayerId(),
            containerId,
            containerSlot);

  auto* container = getContainer(playerCtrl, containerId);
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

void ContainerManager::addItem(const PlayerCtrl* playerCtrl, int containerId, int containerSlot, const Item& item)
{
  LOG_DEBUG("%s: playerId: %d, containerId: %d, containerSlot: %d, itemId: %d",
            __func__,
            playerCtrl->getPlayerId(),
            containerId,
            containerSlot,
            item.getItemId());

  auto* container = getContainer(playerCtrl, containerId);
  if (!container)
  {
    // getContainer logs error
    return;
  }

  if (containerId != container->id)
  {
    LOG_DEBUG("%s: clientContainerId: %d -> containerId: %d",
              __func__,
              containerId,
              container->id);
  }

  // If the containerSlot is valid, then check if the item it points to is a container
  if (containerSlot >= 0 &&
      containerSlot < static_cast<int>(container->items.size()) &&
      container->items[containerSlot].isContainer())
  {
    // We might need to make a new Container object for the inner container
    if (container->items[containerSlot].getContainerId() == Container::INVALID_ID)
    {
      LOG_ERROR("%s: create new Container for inner container NOT YET IMPLEMENTED", __func__);
      return;
    }

    // Change the container pointer to the inner container
    container = &containers_.at(container->items[containerSlot].getContainerId());
  }

  // Add the item at the front
  container->items.insert(container->items.begin(), item);

  // Inform players that have this contianer open about the change
  for (auto& relatedPlayer : container->relatedPlayers)
  {
    relatedPlayer.playerCtrl->onContainerAddItem(relatedPlayer.clientContainerId, item);
  }
}

void ContainerManager::openContainer(PlayerCtrl* playerCtrl,
                                     Container* container,
                                     int clientContainerId,
                                     const Item& item)
{
  LOG_DEBUG("%s: playerId: %d, containerId: %d, clientContainerId: %d, itemId: %d",
            __func__,
            playerCtrl->getPlayerId(),
            container->id,
            clientContainerId,
            item.getItemId());

  // Check if there already is an open container at the given clientContainerId
  const auto currentContainerId = getContainerId(playerCtrl->getPlayerId(), clientContainerId);
  if (currentContainerId != Container::INVALID_ID)
  {
    // Remove player from relatedPlayers
    auto& currentContainer = containers_.at(currentContainerId);
    removeRelatedPlayer(&currentContainer, playerCtrl, clientContainerId);
  }

  // Add player to related players
  addRelatedPlayer(container, playerCtrl, clientContainerId);

  // Set clientContainerId
  setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, container->id);

  // Send onOpenContainer
  playerCtrl->onOpenContainer(clientContainerId, *container, item);
}

void ContainerManager::closeContainer(PlayerCtrl* playerCtrl, Container* container, int clientContainerId)
{
  LOG_DEBUG("%s: playerId: %d, containerId: %d, clientContainerId: %d",
            __func__,
            playerCtrl->getPlayerId(),
            container->id,
            clientContainerId);

  // Remove player from related players
  removeRelatedPlayer(container, playerCtrl, clientContainerId);

  // Unset clientContainerId
  setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, Container::INVALID_ID);

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
  if (isClientContainerId(containerId))
  {
    LOG_ERROR("%s: called with clientContainerId: %d", __func__, containerId);
    return Container::INVALID_ID;
  }

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
    return Container::INVALID_ID;
  }
}

int ContainerManager::getContainerId(CreatureId playerId, int clientContainerId) const
{
  if (!isClientContainerId(clientContainerId))
  {
    LOG_ERROR("%s: called with (global) containerId: %d", __func__, clientContainerId);
    return Container::INVALID_ID;
  }

  const auto& clientContainerIds = clientContainerIds_.at(playerId);
  return clientContainerIds[clientContainerId];
}

void ContainerManager::addRelatedPlayer(Container* container, PlayerCtrl* playerCtrl, int clientContainerId)
{
  container->relatedPlayers.emplace_back(playerCtrl, clientContainerId);
}

void ContainerManager::removeRelatedPlayer(Container* container, const PlayerCtrl* playerCtrl, int clientContainerId)
{
  auto it = std::find_if(container->relatedPlayers.begin(),
                         container->relatedPlayers.end(),
                         [playerCtrl, clientContainerId](const auto& relatedPlayer)
  {
    return relatedPlayer.playerCtrl->getPlayerId() == playerCtrl->getPlayerId() &&
           relatedPlayer.clientContainerId == clientContainerId;
  });
  if (it == container->relatedPlayers.end())
  {
    LOG_ERROR("%s: could not find RelatedPlayer with playerId: %d, clientContainerId: %d in container: %d",
              __func__,
              playerCtrl->getPlayerId(),
              clientContainerId,
              container->id);
    return;
  }

  container->relatedPlayers.erase(it);
}
