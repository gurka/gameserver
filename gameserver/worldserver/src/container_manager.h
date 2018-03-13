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

#ifndef WORLDSERVER_CONTAINERMANAGER_H_
#define WORLDSERVER_CONTAINERMANAGER_H_

#include <algorithm>
#include <vector>

#include "item.h"
#include "position.h"
#include "logger.h"

struct Container
{
  Container()
    : id(INVALID_ID),
      parentContainerId(INVALID_ID),
      rootContainerId(INVALID_ID),
      weight(0),
      containerItemId(Item::INVALID_ID),
      position(),
      items(),
      relatedPlayers()
  {
  }

  static constexpr int INVALID_ID = 0;
  static constexpr int PARENT_IS_TILE = 1;
  static constexpr int PARENT_IS_PLAYER = 2;
  static constexpr int VALID_ID_START = 3;

  // This Container's id
  int id;

  // This Container's parent's id or PARENT_IS_TILE / PARENT_IS_PLAYER if no parent
  int parentContainerId;

  // The root Container's id, PARENT_IS_TILE or PARENT_IS_PLAYER
  int rootContainerId;

  // The total weight of this Container and all Items in it (including other Containers)
  int weight;

  // The ItemId of this Container (e.g. ItemId of a backpack)
  ItemId containerItemId;

  // Position of the Item that this Container belongs to, only applicable if parentContainerId is PARENT_IS_TILE
  Position position;

  // Collection of Items in the Container
  std::vector<Item> items;

  // List of Players that have this Container open
  std::vector<CreatureId> relatedPlayers;
};


class ContainerManager
{
 public:
  ContainerManager()
    : nextContainterId_(Container::VALID_ID_START),
      containers_()
  {
  }

  // Creates a new Container that is located on a Tile at the given position
  int createNewContainer(ItemId containerItemId, const Position& position)
  {
    containers_.emplace_back();

    auto& container = containers_.back();
    container.id = nextContainterId_;
    container.parentContainerId = Container::PARENT_IS_TILE;
    container.rootContainerId = Container::PARENT_IS_TILE;
    container.weight = Item(containerItemId).getWeight();
    container.containerItemId = containerItemId;
    container.position = position;

    nextContainterId_ += 1;

    return container.id;
  }

  // Creates a new Container that is equipped on a Player
  int createNewContainer(ItemId containerItemId)
  {
    containers_.emplace_back();

    auto& container = containers_.back();
    container.id = nextContainterId_;
    container.parentContainerId = Container::PARENT_IS_PLAYER;
    container.rootContainerId = Container::PARENT_IS_PLAYER;
    container.weight = Item(containerItemId).getWeight();
    container.containerItemId = containerItemId;

    nextContainterId_ += 1;

    return container.id;
  }

  const Container& getContainer(int containerId) const
  {
    for (auto& container : containers_)
    {
      if (container.id == containerId)
      {
        return container;
      }
    }

    static const Container INVALID_CONTAINER = Container();
    return INVALID_CONTAINER;
  }

  void addPlayer(int containerId, CreatureId playerId)
  {
    auto& container = getContainerInternal(containerId);
    if (container.id == Container::INVALID_ID)
    {
      LOG_ERROR("%s: no container with id: %d", __func__, containerId);
      return;
    }
    container.relatedPlayers.push_back(playerId);
  }

  void removePlayer(int containerId, CreatureId playerId)
  {
    auto& container = getContainerInternal(containerId);
    if (container.id == Container::INVALID_ID)
    {
      LOG_ERROR("%s: no container with id: %d", __func__, containerId);
      return;
    }

    auto it = std::find_if(container.relatedPlayers.begin(),
                           container.relatedPlayers.end(),
                           [playerId](CreatureId id){ return id == playerId; });
    if (it == container.relatedPlayers.end())
    {
      LOG_ERROR("%s: playerId: %d not found in relatedPlayers", __func__, playerId);
      return;
    }
    container.relatedPlayers.erase(it);
  }

 private:
  Container& getContainerInternal(int containerId)
  {
    for (auto& container : containers_)
    {
      if (container.id == containerId)
      {
        return container;
      }
    }

    static Container INVALID_CONTAINER = Container();
    return INVALID_CONTAINER;
  }

  int nextContainterId_;
  std::vector<Container> containers_;
};

#endif  // WORLDSERVER_CONTAINERMANAGER_H_
