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
#include <unordered_map>

#include "item.h"
#include "position.h"
#include "logger.h"
#include "container.h"

class ContainerManager
{
 public:
  ContainerManager()
    : nextContainerId_(100),  // Container id starts at 100 so that we can catch errors where
                              // client container id (0..63) are accidentally used as a regular
                              // container id
      containers_()
  {
  }

  int createContainer(const ItemPosition& itemPosition)
  {
    LOG_DEBUG("%s: containerId: %d itemPosition: %s", __func__, nextContainerId_, itemPosition.toString().c_str());

    auto& container = containers_[nextContainerId_];
    container.id = nextContainerId_;
    container.weight = 0;  // TODO(simon): fix
    container.itemPosition = itemPosition;

    // Until we have a database with containers...
    if (container.id == 100)
    {
      container.items = { Item(1712), Item(1745), Item(1411) };
    }
    else if (container.id == 101)
    {
      container.items = { Item(1560) };
    }

    nextContainerId_ += 1;

    return container.id;
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

  void addPlayer(int containerId, CreatureId playerId)
  {
    auto* container = getContainer(containerId);
    if (!container)
    {
      LOG_ERROR("%s: container with id: %d not found", __func__, containerId);
      return;
    }

    container->relatedPlayers.push_back(playerId);
  }

  void removePlayer(int containerId, CreatureId playerId)
  {
    auto* container = getContainer(containerId);
    if (!container)
    {
      LOG_ERROR("%s: container with id: %d not found", __func__, containerId);
      return;
    }

    auto it = std::find_if(container->relatedPlayers.begin(),
                           container->relatedPlayers.end(),
                           [playerId](CreatureId id){ return id == playerId; });
    if (it == container->relatedPlayers.end())
    {
      LOG_ERROR("%s: playerId: %d not found in relatedPlayers", __func__, playerId);
      return;
    }

    container->relatedPlayers.erase(it);
  }

  void useContainer(Item* item, PlayerCtrl* playerCtrl)
  {
  }

  void closeContainer(Item* item, PlayerCtrl* playerCtrl)
  {
  }

  void openParentContainer(Item* item, PlayerCtrl* playerCtrl)
  {
  }

 private:
  int nextContainerId_;
  std::unordered_map<int, Container> containers_;
};

#endif  // GAMEENGINE_CONTAINERMANAGER_H_
