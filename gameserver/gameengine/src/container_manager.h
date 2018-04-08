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

#ifndef GAMEENGINE_CONTAINERMANAGER_H_
#define GAMEENGINE_CONTAINERMANAGER_H_

#include <algorithm>
#include <array>
#include <iterator>
#include <unordered_map>

#include "item.h"
#include "logger.h"
#include "container.h"
#include "player_ctrl.h"

class ContainerManager
{
 public:
  ContainerManager()
    : nextContainerId_(64),  // Container id starts at 64 so that we can catch errors where
                             // client container id (0..63) are accidentally used as a regular
                             // container id
      containers_(),
      clientContainerIds_()
  {
  }

  Container* getContainer(PlayerCtrl* playerCtrl, int containerId)
  {
    if (containerId == Container::INVALID_ID)
    {
      LOG_ERROR("%s: invalid containerId", __func__);
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

    return &containers_[containerId];
  }

  Item* getItem(PlayerCtrl* playerCtrl, int containerId, int containerSlot)
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

  void useContainer(PlayerCtrl* playerCtrl, Item* item, const ItemPosition& itemPosition, int newClientContainerId)
  {
    if (!item->isContainer())
    {
      LOG_ERROR("%s: item with id %d is not a container", __func__, item->getItemId());
      return;
    }

    if (item->getContainerId() == Container::INVALID_ID)
    {
      // Create new Container
      auto& container = containers_[nextContainerId_];
      container.id = nextContainerId_;
      container.weight = 0;
      container.itemId = item->getItemId();
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

      item->setContainerId(nextContainerId_);
      nextContainerId_ += 1;

      // Until we have a database with containers...
      if (container.id == 64)
      {
        container.items = { Item(1712), Item(1745), Item(1411) };
      }
      else if (container.id == 65)
      {
        container.items = { Item(1560) };
      }
    }

    if (containers_.count(item->getContainerId()) == 0)
    {
      LOG_ERROR("%s: item has invalid containerId (%d)", __func__, item->getContainerId());
      return;
    }

    auto& container = containers_[item->getContainerId()];

    // Check if Player has this Container open
    const auto clientContainerId = getClientContainerId(playerCtrl->getPlayerId(), container.id);
    if (clientContainerId == Container::INVALID_ID)
    {
      openContainer(playerCtrl, &container, newClientContainerId, *item);
    }
    else
    {
      // Do not close the Container here, the client will ack this by sending closeContainer
      playerCtrl->onCloseContainer(clientContainerId);
    }
  }

  void closeContainer(PlayerCtrl* playerCtrl, int containerId)
  {
    if (!isClientContainerId(containerId))
    {
      LOG_ERROR("%s: must be called with clientContainerId", __func__);
      return;
    }

    auto* container = getContainer(playerCtrl, containerId);
    if (!container)
    {
      LOG_ERROR("%s: could not find container with clientContainerId: %d", __func__, containerId);
      return;
    }

    closeContainer(playerCtrl, container, containerId);
  }

  void openParentContainer(PlayerCtrl* playerCtrl, int containerId)
  {
    if (!isClientContainerId(containerId))
    {
      LOG_ERROR("%s: must be called with clientContainerId", __func__);
      return;
    }

    auto* currentContainer = getContainer(playerCtrl, containerId);
    if (!currentContainer)
    {
      LOG_ERROR("%s: no container found with id: %d", __func__, containerId);
      return;
    }

    auto* parentContainer = getContainer(playerCtrl, currentContainer->parentContainerId);
    if (!parentContainer)
    {
      LOG_ERROR("%s: no container found with id: %d", __func__, currentContainer->parentContainerId);
      return;
    }

    const auto parentContainerItem = Item(parentContainer->itemId);
    openContainer(playerCtrl, parentContainer, containerId, parentContainerItem);
  }

  void playerDisconnected(PlayerCtrl* playerCtrl)
  {
    const auto playerId = playerCtrl->getPlayerId();
    if (clientContainerIds_.count(playerId) == 1)
    {
      for (const auto containerId : clientContainerIds_[playerId])
      {
        // This is because the array in clientContainerIds_ is constructed with default values
        // zero, which doesn't match ContainerId::INVALID_ID (-1)...
        if (containerId < 64)
        {
          continue;
        }

        auto& container = containers_.at(containerId);
        const auto it = std::find(container.relatedPlayers.cbegin(), container.relatedPlayers.cend(), playerCtrl);
        if (it == container.relatedPlayers.cend())
        {
          LOG_ERROR("%s: player: %d not found in relatedPlayers", __func__, playerId);
        }
        container.relatedPlayers.erase(it);
      }

      clientContainerIds_.erase(playerId);
    }
  }

 private:
  bool isClientContainerId(int containerId) const
  {
    return containerId >= 0 && containerId < 64;
  }

  void openContainer(PlayerCtrl* playerCtrl, Container* container, int clientContainerId, const Item& item)
  {
    LOG_DEBUG("%s: playerId: %d, containerId: %d, clientContainerId: %d, itemId: %d",
              __func__,
              playerCtrl->getPlayerId(),
              container->id,
              clientContainerId,
              item.getItemId());

    // Add player to related players
    container->relatedPlayers.push_back(playerCtrl);

    // Set clientContainerId
    setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, container->id);

    // Send onOpenContainer
    playerCtrl->onOpenContainer(clientContainerId, *container, item);
  }

  void closeContainer(PlayerCtrl* playerCtrl, Container* container, int clientContainerId)
  {
    LOG_DEBUG("%s: playerId: %d, containerId: %d, clientContainerId: %d",
              __func__,
              playerCtrl->getPlayerId(),
              container->id,
              clientContainerId);

    // Remove player from related players
    const auto it = std::find(container->relatedPlayers.cbegin(), container->relatedPlayers.cend(), playerCtrl);
    if (it == container->relatedPlayers.cend())
    {
      LOG_ERROR("%s: player: %d not found in relatedPlayers", __func__, playerCtrl->getPlayerId());
    }
    container->relatedPlayers.erase(it);

    // Unset clientContainerId
    setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, Container::INVALID_ID);

    // Send onCloseContainer
    playerCtrl->onCloseContainer(clientContainerId);
  }

  void setClientContainerId(CreatureId playerId, int clientContainerId, int containerId)
  {
    auto& clientContainerIds = clientContainerIds_[playerId];
    clientContainerIds[clientContainerId] = containerId;
  }

  int getClientContainerId(CreatureId playerId, int containerId)
  {
    if (isClientContainerId(containerId))
    {
      LOG_ERROR("%s: called with clientContainerId: %d", __func__, containerId);
      return Container::INVALID_ID;
    }

    const auto& clientContainerIds = clientContainerIds_[playerId];
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

  int getContainerId(CreatureId playerId, int clientContainerId)
  {
    if (!isClientContainerId(clientContainerId))
    {
      LOG_ERROR("%s: called with (global) containerId: %d", __func__, clientContainerId);
      return Container::INVALID_ID;
    }

    const auto& clientContainerIds = clientContainerIds_[playerId];
    return clientContainerIds[clientContainerId];
  }

  int nextContainerId_;
  std::unordered_map<int, Container> containers_;
  std::unordered_map<CreatureId, std::array<int, 64>> clientContainerIds_;
};

#endif  // GAMEENGINE_CONTAINERMANAGER_H_
