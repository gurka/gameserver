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
    Thing(const Creature* creature)
        : creature(creature),
          item(nullptr)
    {
    }

    Thing(const Item* item)
        : creature(nullptr),
          item(item)
    {
    }

    const Creature* creature;
    const Item* item;
  };

  Tile() = default;

  explicit Tile(const Item* groundItem)
  {
    things_.emplace_back(groundItem);
  }

  // Delete copy constructors
  Tile(const Tile&) = delete;
  Tile& operator=(const Tile&) = delete;

  // Move is OK
  Tile(Tile&&) = default;
  Tile& operator=(Tile&&) = default;

  // Creatures (TODO: sort out Creature* vs CreatureId)
  void addCreature(const Creature* creature);
  bool removeCreature(CreatureId creatureId);
  const Creature* getCreature(int stackPosition) const;
  CreatureId getCreatureId(int stackPosition) const;
  int getCreatureStackPos(CreatureId creatureId) const;

  // Items (TODO: sort out Item* vs ItemUniqueId)
  void addItem(const Item& item);
  bool removeItem(int stackPosition);
  const Item* getItem(int stackPosition) const;
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
