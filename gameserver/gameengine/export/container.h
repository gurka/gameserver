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

#ifndef GAMEENGINE_CONTAINER_H_
#define GAMEENGINE_CONTAINER_H_

#include <vector>

#include "container_id.h"
#include "item.h"
#include "game_position.h"
#include "creature.h"

class PlayerCtrl;

struct Container
{
  Container()
    : id(ContainerId::INVALID_ID),
      weight(0),
      itemId(0),
      parentContainerId(ContainerId::INVALID_ID),
      rootItemPosition(),
      items(),
      relatedPlayers()
  {
  }

  // This Container's id
  int id;

  // The total weight of this Container and all Items in it (including other Containers)
  int weight;

  // Id of the Item that corresponds to this Container (e.g. itemId of a backpack)
  int itemId;

  // Container id of the parent container, or ContainerId::INVALID_ID if no parent
  // This id must NOT be a clientContainerId
  ContainerId parentContainerId;

  // ItemPosition of the root item that this Container belongs to
  // Is either a world position or a inventory position
  ItemPosition rootItemPosition;

  // Collection of Items in the Container
  std::vector<Item> items;

  // List of Players that have this Container open
  std::vector<PlayerCtrl*> relatedPlayers;
};

#endif // GAMEENGINE_CONTAINER_H_
