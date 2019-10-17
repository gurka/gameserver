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

namespace server
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
  std::deque<common::Direction> path;
};

struct MoveItem  // or MoveThing?
{
  common::ItemPosition from_item_position;
  common::GamePosition to_game_position;
  std::uint8_t count;
};

struct UseItem
{
  common::ItemPosition item_position;
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
  common::ItemPosition item_position;
};

struct Say
{
  std::uint8_t type;
  std::string receiver;
  std::uint16_t channel_id;
  std::string message;
};

}  // namespace server

namespace client
{

struct Login
{
  std::uint32_t player_id;
  std::uint16_t server_beat;
};

struct LoginFailed
{
  std::string reason;
};

struct Creature
{
  bool known;
  std::uint32_t id_to_remove;  // only if known = false
  std::uint32_t id;
  std::string name;  // only if known = false
  std::uint8_t health_percent;
  common::Direction direction;
  common::Outfit outfit;
  std::uint16_t speed;
};

struct Item
{
  std::uint16_t item_type_id;
  std::uint8_t extra;  // only if type is stackable or multitype
};

struct Equipment
{
  bool empty;
  std::uint8_t inventory_index;
  Item item;  // only if empty = false
};

struct MagicEffect
{
  common::Position position = { 0, 0, 0 };
  std::uint8_t type;
};

struct PlayerStats
{
  std::uint16_t health;
  std::uint16_t max_health;
  std::uint16_t capacity;
  std::uint32_t exp;
  std::uint8_t level;
  std::uint16_t mana;
  std::uint16_t max_mana;
  std::uint8_t magic_level;
};

struct WorldLight
{
  std::uint8_t intensity;
  std::uint8_t color;
};

struct PlayerSkills
{
  std::uint8_t fist;
  std::uint8_t club;
  std::uint8_t sword;
  std::uint8_t axe;
  std::uint8_t dist;
  std::uint8_t shield;
  std::uint8_t fish;
};

struct TextMessage
{
  std::uint8_t type;
  std::string message;
};

struct MapData
{
  struct CreatureData
  {
    Creature creature;
    std::uint8_t stackpos;
  };

  struct ItemData
  {
    Item item;
    std::uint8_t stackpos;
  };

  struct TileData
  {
    bool skip;
    std::vector<CreatureData> creatures;
    std::vector<ItemData> items;
  };

  common::Position position = { 0, 0, 0 };
  std::vector<TileData> tiles;
};

struct CreatureMove
{
  bool can_see_old_pos;
  bool can_see_new_pos;

  common::Position old_position = { 0, 0, 0 };  // only if canSeeOldPos = true
  std::uint8_t old_stackpos;                    // only if canSeeOldPos = true
  common::Position new_position = { 0, 0, 0 };  // only if canSeeNewPos = true
  Creature creature;                            // only if canSeeOldPos = false and canSeeNewPos = true
};

}  // namespace client

}  // namespace protocol

#endif  // PROTOCOL_EXPORT_PROTOCOL_TYPES_H_
