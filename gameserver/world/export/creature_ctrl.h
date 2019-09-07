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

#ifndef WORLD_EXPORT_CREATURE_CTRL_H_
#define WORLD_EXPORT_CREATURE_CTRL_H_

#include <string>

class WorldInterface;
class Creature;
class Position;
class Tile;
class Item;

class CreatureCtrl
{
 public:
  virtual ~CreatureCtrl() = default;

  // Called when the creature has spawned nearby this creature
  // Can be the creature itself that has spawned
  virtual void onCreatureSpawn(const WorldInterface& world_interface,
                               const Creature& creature,
                               const Position& position) = 0;

  // Called when a creature has despawned nearby this creature
  virtual void onCreatureDespawn(const WorldInterface& world_interface,
                                 const Creature& creature,
                                 const Position& position,
                                 std::uint8_t stackPos) = 0;

  // Called when a creature has moved
  virtual void onCreatureMove(const WorldInterface& world_interface,
                              const Creature& creature,
                              const Position& oldPosition,
                              std::uint8_t oldStackPos,
                              const Position& newPosition) = 0;

  // Called when a creature has turned
  virtual void onCreatureTurn(const WorldInterface& world_interface,
                              const Creature& creature,
                              const Position& position,
                              std::uint8_t stackPos) = 0;

  // Called when a creature says something
  virtual void onCreatureSay(const WorldInterface& world_interface,
                             const Creature& creature,
                             const Position& position,
                             const std::string& message) = 0;

  // Called when an Item was removed from a Tile
  virtual void onItemRemoved(const WorldInterface& world_interface,
                             const Position& position,
                             std::uint8_t stackPos) = 0;

  // Called when an Item was added to a Tile
  virtual void onItemAdded(const WorldInterface& world_interface,
                           const Item& item,
                           const Position& position) = 0;

  // Called when a Tile has been updated
  virtual void onTileUpdate(const WorldInterface& world_interface,
                            const Position& position) = 0;
};

#endif  // WORLD_EXPORT_CREATURE_CTRL_H_
