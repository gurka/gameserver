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

#ifndef PROTOCOL_EXPORT_PROTOCOL_TYPES_H_
#define PROTOCOL_EXPORT_PROTOCOL_TYPES_H_

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "direction.h"
#include "position.h"
#include "game_position.h"

namespace protocol
{

struct Login
{
  std::uint8_t unknown1;
  std::uint8_t client_os;
  std::uint16_t client_version;
  std::uint8_t unknown2;
  std::string character_name;
  std::string password;
};

struct MoveClick
{
  std::deque<world::Direction> path;
};

struct MoveItem  // or MoveThing?
{
  gameengine::ItemPosition from_item_position;
  gameengine::GamePosition to_game_position;
  std::uint8_t count;
};

struct UseItem
{
  gameengine::ItemPosition item_position;
  std::uint8_t new_container_id;
};

struct CloseContainer
{
  std::uint8_t container_id;
};

struct OpenParentContainer
{
  std::uint8_t container_id;
};

struct LookAt
{
  gameengine::ItemPosition item_position;
};

struct Say
{
  std::uint8_t type;
  std::string receiver;
  std::uint16_t channel_id;
  std::string message;
};

}  // namespace protocol

#endif  // PROTOCOL_EXPORT_PROTOCOL_TYPES_H_
