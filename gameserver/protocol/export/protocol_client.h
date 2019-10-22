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

#ifndef PROTOCOL_EXPORT_PROTOCOL_CLIENT_H_
#define PROTOCOL_EXPORT_PROTOCOL_CLIENT_H_

#include <cstdint>
#include <string>

#include "protocol_common.h"

namespace protocol::client
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

struct ThingAdded
{
  //common::Position position;
  //Thing thing;
};

struct ThingChanged
{

};

struct ThingRemoved
{

};

struct ThingMoved
{

};

struct Map
{
  common::Position position = { 0, 0, 0 };
  std::vector<Tile> tiles;
};

// Reading packets

// 0x0A
Login getLogin(network::IncomingPacket* packet);

// 0x14
LoginFailed getLoginFailed(network::IncomingPacket* packet);

// 0x64
Map getMap(int width, int height, network::IncomingPacket* packet);

// 0x83
MagicEffect getMagicEffect(network::IncomingPacket* packet);

// 0x78 0x79
Equipment getEquipment(bool empty, network::IncomingPacket* packet);

// 0x0A
PlayerStats getPlayerStats(network::IncomingPacket* packet);

// 0x82
WorldLight getWorldLight(network::IncomingPacket* packet);

// 0xA1
PlayerSkills getPlayerSkills(network::IncomingPacket* packet);

// 0xB4
TextMessage getTextMessage(network::IncomingPacket* packet);

// 0x6A
ThingAdded getThingAdded(network::IncomingPacket* packet);

// 0x6B
ThingChanged getThingChanged(network::IncomingPacket* packet);

// 0x6C
ThingRemoved getThingRemoved(network::IncomingPacket* packet);

// 0x6D
ThingMoved getThingMoved(network::IncomingPacket* packet);

}  // namespace protocol::client

#endif  // PROTOCOL_EXPORT_PROTOCOL_CLIENT_H_
