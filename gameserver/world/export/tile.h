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
  struct Thing
  {
    Thing(CreatureId creatureId_)
    {
      isItem = false;
      creatureId = creatureId_;
    }

    Thing(ItemUniqueId itemUniqueId, bool onTop)
    {
      isItem = true;
      item.itemUniqueId = itemUniqueId;
      item.onTop = onTop;
    }

    bool isItem;
    union
    {
      struct
      {
        ItemUniqueId itemUniqueId;
        bool onTop;
      } item;
      CreatureId creatureId;
    };
  };

  explicit Tile(ItemUniqueId groundItem)
  {
    things_.emplace_back(groundItem, false);
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
  int getCreatureStackPos(CreatureId creatureId) const;

  // Items
  void addItem(ItemUniqueId item, bool onTop);
  bool removeItem(int stackPosition);
  ItemUniqueId getItemUniqueId(int stackPosition) const;

  // Other
  const std::vector<Thing>& getThings() const { return things_; }
  std::size_t getNumberOfThings() const { return things_.size(); }

 private:
  // First ground
  // Then onTop items
  // Then creatures
  // Then other items
  std::vector<Thing> things_;
};

#endif  // WORLD_EXPORT_TILE_H_
