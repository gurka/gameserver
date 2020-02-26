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

#ifndef GAMEENGINE_EXPORT_CONTAINER_H_
#define GAMEENGINE_EXPORT_CONTAINER_H_

#include <sstream>
#include <string>
#include <vector>

#include "game_position.h"

namespace common
{

class Item;

}

namespace gameengine
{

class PlayerCtrl;

struct Container
{
  Container()
    : item(nullptr),
      parent_item_unique_id(common::Item::INVALID_UNIQUE_ID)
  {
  }

  // Disallow copy and move, Container should only reside in ContainerManager::containers_
  Container(const Container&) = delete;
  Container& operator=(const Container&) = delete;
  Container(Container&&) = delete;
  Container& operator=(Container&&) = delete;

  // The total weight of this Container and all Items in it (including other Containers)
  int weight{0};

  // The Item that corresponds to this Container
  const common::Item* item{nullptr};

  // Container id of the parent container, or INVALID_ID if no parent
  common::ItemUniqueId parent_item_unique_id;

  // GamePosition of the root item that this Container belongs to
  // Is either a world position or an inventory position
  // Note: changed from ItemPosition to GamePosition, is it OK?
  common::GamePosition root_game_position;

  // Collection of Items in the Container
  std::vector<const common::Item*> items;

  // List of Players that have this Container open
  std::vector<PlayerCtrl*> related_players;

  std::string toString(int indent = 2) const;
};

}  // namespace gameengine

#endif  // GAMEENGINE_EXPORT_CONTAINER_H_
