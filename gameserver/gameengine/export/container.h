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

#include <sstream>
#include <string>
#include <vector>

#include "game_position.h"
#include "player_ctrl.h"

class Item;

struct Container
{
  Container()
    : weight(0),
      item(nullptr),
      parentItemUniqueId(Item::INVALID_UNIQUE_ID),
      rootGamePosition(),
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
  const Item* item;

  // Container id of the parent container, or INVALID_ID if no parent
  ItemUniqueId parentItemUniqueId;

  // GamePosition of the root item that this Container belongs to
  // Is either a world position or an inventory position
  // Note: changed from ItemPosition to GamePosition, is it OK?
  GamePosition rootGamePosition;

  // Collection of Items in the Container
  std::vector<const Item*> items;

  // List of Players that have this Container open
  std::vector<PlayerCtrl*> relatedPlayers;

  std::string toString(int indent = 2) const
  {
    const auto indents = std::string(indent, ' ');
    std::ostringstream ss;
    ss << indents << "Weight:             " << weight << '\n';
    ss << indents << "ItemUniqueId:       " << item->getItemUniqueId()
       << " (" << (item->getItemType().isContainer ? "" : "not ") << "container)\n";
    ss << indents << "parentItemUniqueId: " << parentItemUniqueId << '\n';
    ss << indents << "rootGamePosition:   " << rootGamePosition.toString() << '\n';
    ss << indents << "items:\n";
    for (const auto* item : items)
    {
      ss << indents << indents << "ItemUniqueId: " << item->getItemUniqueId()
         << " (" << (item->getItemType().isContainer ? "" : "not ") << "container)\n";
    }
    ss << indents << "relatedPlayers:\n";
    for (const auto* playerCtrl : relatedPlayers)
    {
      ss << indents << indents << "PlayerID: " << playerCtrl->getPlayerId() << '\n';
    }
    return ss.str();
  }
};

#endif  // GAMEENGINE_EXPORT_CONTAINER_H_
