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

#ifndef GAMEENGINE_EXPORT_CONTAINER_H_
#define GAMEENGINE_EXPORT_CONTAINER_H_

#include <vector>

#include "game_position.h"

class Item;
class PlayerCtrl;

struct Container
{
  Container()
    : weight(0),
      item(nullptr),
      parentItemUniqueId(Item::INVALID_UNIQUE_ID),
      rootItemPosition(),
      items(),
      relatedPlayers()
  {
  }

  // Disallow copy and move, Container should only reside in ContainerManager::containers_
  Container(const Container&) = delete;
  Container& operator=(const Container&) = delete;
  Container(Container&&) = delete;
  Container& operator=(Container&&) = delete;

  // The total weight of this Container and all Items in it (including other Containers)
  int weight;

  // The Item that corresponds to this Container
  // TODO(simon): store ItemUniqueId instead?
  const Item* item;

  // Container id of the parent container, or INVALID_ID if no parent
  // This id must NOT be a clientContainerId
  ItemUniqueId parentItemUniqueId;

  // ItemPosition of the root item that this Container belongs to
  // Is either a world position or an inventory position
  ItemPosition rootItemPosition;

  // Collection of Items in the Container
  // TODO(simon): store ItemUniqueId instead?
  std::vector<Item*> items;

  // List of Players that have this Container open
  struct RelatedPlayer
  {
    RelatedPlayer(PlayerCtrl* playerCtrl, int clientContainerId)
      : playerCtrl(playerCtrl),
        clientContainerId(clientContainerId)
    {
    }

    bool operator==(const RelatedPlayer& other) const
    {
      return playerCtrl == other.playerCtrl &&
             clientContainerId == other.clientContainerId;
    }

    bool operator!=(const RelatedPlayer& other) const
    {
      return !(*this == other);
    }

    PlayerCtrl* playerCtrl;
    int clientContainerId;
  };
  std::vector<RelatedPlayer> relatedPlayers;
};

#endif  // GAMEENGINE_EXPORT_CONTAINER_H_
