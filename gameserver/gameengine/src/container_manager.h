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
#include "position.h"
#include "logger.h"
#include "container.h"
#include "player_ctrl.h"

class ContainerManager
{
 public:
  ContainerManager()
    : nextContainerId_(100),  // Container id starts at 100 so that we can catch errors where
                              // client container id (0..63) are accidentally used as a regular
                              // container id
      containers_(),
      clientContainerIds_()
  {
  }

  Container* getContainer(int containerId)
  {
    if (containerId < 100)
    {
      LOG_ERROR("%s: containerId: %d is a client container id", __func__, containerId);
    }

    if (containers_.count(containerId) == 1)
    {
      return &containers_.at(containerId);
    }

    return nullptr;
  }

  Item* getItem(int containerId, int containerSlot)
  {
    auto* container = getContainer(containerId);
    if (!container)
    {
      LOG_ERROR("%s: container with id: %d not found", __func__, containerId);
      return nullptr;
    }

    if (containerSlot < 0 || containerSlot >= static_cast<int>(container->items.size()))
    {
      LOG_ERROR("%s: containerSlot: %d out of range for container with id: %d",
                __func__,
                containerSlot,
                containerId);
      return nullptr;
    }

    return &container->items[containerSlot];
  }

  Item* getItem(PlayerCtrl* playerCtrl, const ItemPosition& itemPosition)
  {
    return nullptr;
  }

  void useContainer(PlayerCtrl* playerCtrl, Item* item, int newClientContainerId)
  {
    if (!item->isContainer())
    {
      LOG_ERROR("%s: item with id %d is not a container", __func__, item->getItemId());
      return;
    }

    if (item->getContainerId() == ContainerId::INVALID_ID)
    {
      // Create new Container
      LOG_DEBUG("%s: creating new Container with id %d", __func__, nextContainerId_);
      auto& container = containers_[nextContainerId_];
      container.id = nextContainerId_;
      item->setContainerId(nextContainerId_);
      nextContainerId_ += 1;

      // Until we have a database with containers...
      if (container.id == 100)
      {
        container.items = { Item(1712), Item(1745), Item(1411) };
      }
      else if (container.id == 101)
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
    if (clientContainerId == ContainerId::INVALID_ID)
    {
      openContainer(playerCtrl, &container, newClientContainerId, item);
    }
    else
    {
      // Do not close the Container here, the client will ack this by sending closeContainer
      playerCtrl->onCloseContainer(clientContainerId);
    }
  }

  void closeContainer(PlayerCtrl* playerCtrl, ContainerId containerId)
  {
    if (!containerId.isClientContainerId())
    {
      LOG_ERROR("%s: must be called with clientContainerId", __func__);
      return;
    }

    const auto& clientContainerIds = clientContainerIds_[playerCtrl->getPlayerId()];
    if (clientContainerIds[containerId.getContainerId()] == ContainerId::INVALID_ID)
    {
      LOG_ERROR("%s: clientContainerId: %d is invalid", __func__, containerId.getContainerId());
      return;
    }

    auto* container = getContainer(clientContainerIds[containerId.getContainerId()]);

    closeContainer(playerCtrl, container, containerId.getContainerId());
  }

  void openParentContainer(PlayerCtrl* playerCtrl, ContainerId containerId)
  {
  }

 private:
  void openContainer(PlayerCtrl* playerCtrl, Container* container, int clientContainerId, Item* item)
  {
    LOG_DEBUG("%s: playerId: %d, containerId: %d, clientContainerId: %d, itemId: %d",
              __func__,
              playerCtrl->getPlayerId(),
              container->id,
              clientContainerId,
              item->getItemId());

    // Add player to related players
    container->relatedPlayers.push_back(playerCtrl);

    // Set clientContainerId
    setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, container->id);

    // Send onOpenContainer
    playerCtrl->onOpenContainer(clientContainerId, *container, *item);
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
    setClientContainerId(playerCtrl->getPlayerId(), clientContainerId, ContainerId::INVALID_ID);

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
      return ContainerId::INVALID_ID;
    }
  }

  int nextContainerId_;
  std::unordered_map<int, Container> containers_;
  std::unordered_map<CreatureId, std::array<int, 64>> clientContainerIds_;
};

#endif  // GAMEENGINE_CONTAINERMANAGER_H_
