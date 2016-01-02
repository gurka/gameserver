/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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

#ifndef WORLD_TILE_H_
#define WORLD_TILE_H_

#include <deque>
#include <vector>
#include "item.h"
#include "creature.h"

class Tile
{
 public:
  explicit Tile(const Item& groundItem)
    : groundItem_(groundItem)
  {
  }

  // Ground Item
  const Item& getGroundItem() const { return groundItem_; }

  // Creatures
  void addCreature(CreatureId creatureId);
  bool removeCreature(CreatureId creatureId);
  CreatureId getCreatureId(int stackPosition) const;
  const std::deque<CreatureId>& getCreatureIds() const { return creatureIds_; }
  uint8_t getCreatureStackPos(CreatureId creatureId) const;

  // Other Items
  void addItem(const Item& item);
  bool removeItem(ItemId itemId, uint8_t stackPosition);
  Item getItem(uint8_t stackPos) const;
  std::vector<Item> getItems() const;
  const std::deque<Item>& getTopItems() const { return topItems_; }
  const std::deque<Item>& getBottomItems() const { return bottomItems_; }

  std::size_t getNumberOfThings() const;

 private:
  // TODO(gurka): Store all items in a single container, to optimize getItems() ?
  Item groundItem_;
  std::deque<Item> topItems_;
  std::deque<CreatureId> creatureIds_;
  std::deque<Item> bottomItems_;
};

#endif  // WORLD_TILE_H_
