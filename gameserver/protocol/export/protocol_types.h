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

#ifndef WORLDSERVER_SRC_PROTOCOL_TYPES_H_
#define WORLDSERVER_SRC_PROTOCOL_TYPES_H_

#include <cstdint>

#include "direction.h"
#include "creature.h"
#include "position.h"

namespace ProtocolTypes
{

struct Creature
{
  bool known;
  std::uint32_t idToRemove;  // only if known = false
  std::uint32_t id;
  std::string name;  // only if known = false
  std::uint8_t healthPercent;
  Direction direction;
  Outfit outfit;
  std::uint16_t speed;
};

struct Item
{
  std::uint16_t itemTypeId;
  std::uint8_t extra;  // only if type is stackable or multitype
};

struct Equipment
{
  bool empty;
  std::uint8_t inventoryIndex;
  Item item;  // only if empty = false
};

struct MagicEffect
{
  Position position;
  std::uint8_t type;
};

struct PlayerStats
{
  std::uint16_t health;
  std::uint16_t maxHealth;
  std::uint16_t capacity;
  std::uint32_t exp;
  std::uint8_t level;
  std::uint16_t mana;
  std::uint16_t maxMana;
  std::uint8_t magicLevel;
};

}

#endif  // WORLDSERVER_SRC_PROTOCOL_TYPES_H_
