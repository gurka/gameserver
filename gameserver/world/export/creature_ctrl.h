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

#ifndef WORLD_EXPORT_CREATURE_CTRL_H_
#define WORLD_EXPORT_CREATURE_CTRL_H_

#include <string>

namespace common
{

class Creature;
class Position;
class Tile;
class Item;

}

namespace world
{

class CreatureCtrl
{
 public:
  virtual ~CreatureCtrl() = default;

  // Called when the creature has spawned nearby this creature
  // Can be the creature itself that has spawned
  virtual void onCreatureSpawn(const common::Creature& creature,
                               const common::Position& position) = 0;

  // Called when a creature has despawned nearby this creature
  virtual void onCreatureDespawn(const common::Creature& creature,
                                 const common::Position& position,
                                 std::uint8_t stackpos) = 0;

  // Called when a creature has moved
  virtual void onCreatureMove(const common::Creature& creature,
                              const common::Position& old_position,
                              std::uint8_t old_stackpos,
                              const common::Position& new_position) = 0;

  // Called when a creature has turned
  virtual void onCreatureTurn(const common::Creature& creature,
                              const common::Position& position,
                              std::uint8_t stackpos) = 0;

  // Called when a creature says something
  virtual void onCreatureSay(const common::Creature& creature,
                             const common::Position& position,
                             const std::string& message) = 0;

  // Called when an Item was removed from a Tile
  virtual void onItemRemoved(const common::Position& position,
                             std::uint8_t stackpos) = 0;

  // Called when an Item was added to a Tile
  virtual void onItemAdded(const common::Item& item,
                           const common::Position& position) = 0;

  // Called when a Tile has been updated
  virtual void onTileUpdate(const common::Position& position) = 0;
};

}  // namespace world

#endif  // WORLD_EXPORT_CREATURE_CTRL_H_
