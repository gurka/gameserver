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

#ifndef WSCLIENT_SRC_GAME_TYPES_H_
#define WSCLIENT_SRC_GAME_TYPES_H_

#include <cstdint>

#include "common/creature.h"
#include "common/direction.h"

namespace game
{

struct Creature
{
  common::CreatureId id;
  std::string name;
  std::uint8_t health_percent;
  common::Direction direction;
  common::Outfit outfit;
  std::uint16_t speed;
};

struct Item
{
  const common::ItemType* type;
  std::uint8_t extra;
};

// TODO(simon): CreatureId is 32 bits and ItemTypeId+extra is 16+8 bits
//              We could use Thing = std::uint32_t
//              if we can figure out the valid range of creature ids
using Thing = std::variant<common::CreatureId, Item>;

}  // namespace model

#endif  // WSCLIENT_SRC_GAME_TYPES_H_
