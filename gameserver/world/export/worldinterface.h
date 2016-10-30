/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
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

#ifndef WORLD_WORLDINTERFACE_H_
#define WORLD_WORLDINTERFACE_H_

#include <list>
#include <string>
#include "creature.h"

class Tile;
class Position;

class WorldInterface
{
 public:
  virtual ~WorldInterface() = default;

  virtual const std::list<const Tile*> getMapBlock(const Position& position, int width, int height) const = 0;
  virtual const Tile& getTile(const Position& position) const = 0;
  virtual const Creature& getCreature(CreatureId creatureId) const = 0;
  virtual const Position& getCreaturePosition(CreatureId creatureId) const = 0;
};

#endif  // WORLD_WORLDINTERFACE_H_
