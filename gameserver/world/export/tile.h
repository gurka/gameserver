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

#ifndef WORLD_EXPORT_TILE_H_
#define WORLD_EXPORT_TILE_H_

#include <vector>

#include "item.h"
#include "creature.h"

class Tile
{
 public:
  explicit Tile(const Item& groundItem)
    : numberOfTopItems(0),
      items_({groundItem})
  {
  }

  // Delete copy constructors
  Tile(const Tile&) = delete;
  Tile& operator=(const Tile&) = delete;

  // Move is OK
  Tile(Tile&&) = default;
  Tile& operator=(Tile&&) = default;

  // Creatures
  void addCreature(CreatureId creatureId);
  bool removeCreature(CreatureId creatureId);
  CreatureId getCreatureId(int stackPosition) const;
  const std::vector<CreatureId>& getCreatureIds() const { return creatureIds_; }
  int getCreatureStackPos(CreatureId creatureId) const;

  // Items
  void addItem(const Item& item);
  bool removeItem(ItemId itemId, int stackPosition);
  const Item* getItem(int stackPosition) const;
  Item* getItem(int stackPosition);
  const std::vector<Item>& getItems() const { return items_; }

  // Other
  std::size_t getNumberOfThings() const;
  int getGroundSpeed() const;

 private:
  int numberOfTopItems;
  std::vector<Item> items_;
  std::vector<CreatureId> creatureIds_;
};

#endif  // WORLD_EXPORT_TILE_H_
