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

#ifndef WORLD_EXPORT_TILE_H_
#define WORLD_EXPORT_TILE_H_

#include <functional>
#include <vector>

#include "thing.h"
#include "item.h"
#include "creature.h"

namespace world
{

class Tile
{
 public:
  Tile() = default;

  explicit Tile(const common::Item* ground_item)
  {
    m_things.emplace_back(ground_item);
  }

  // Delete copy constructors
  Tile(const Tile&) = delete;
  Tile& operator=(const Tile&) = delete;

  // Move is OK
  Tile(Tile&&) = default;
  Tile& operator=(Tile&&) = default;

  // Thing management
  void addThing(const common::Thing& thing);
  bool removeThing(int stackpos);
  const std::vector<common::Thing>& getThings() const { return m_things; }
  std::size_t getNumberOfThings() const { return m_things.size(); }

  const common::Creature* getCreature(int stackpos) const;
  const common::Item* getItem(int stackpos) const;

  // Helpers
  bool isBlocking() const;
  int getCreatureStackpos(common::CreatureId creature_id) const;

 private:
  // First ground
  // Then onTop items
  // Then creatures
  // Then other items
  std::vector<common::Thing> m_things;
};

}  // namespace world

#endif  // WORLD_EXPORT_TILE_H_
