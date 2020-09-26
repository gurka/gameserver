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
#include "protocol_client.h"

#include <cstdint>

#include "logger.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"

namespace
{

std::vector<protocol::Tile> getMapData(int z, int width, int height, network::IncomingPacket* packet)
{
  std::vector<protocol::Tile> tiles;

  // Valid z is 0..15, 0 is highest and 15 is lowest. 7 is sea level.
  // If on ground or higher (z <= 7) then go over everything above ground (from 7 to 0)
  // If underground (z > 7) then go from two below to two above, with cap on lowest level (from e.g. 8 to 12, if z = 10)
  const auto z_start = z > 7 ? (z - 2)             :  7;
  const auto z_end   = z > 7 ? std::min(z + 2, 15) :  0;
  const auto z_step  = z > 7 ? 1                   : -1;

  // How many floors do we receive?
  //  z=0 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=1 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=2 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=3 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=4 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=5 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=6 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=7 -> 8 floors;  7  6  5  4  3  2  1  0
  //  z=8 -> 5 floors:  6  7  8  9 10
  //  z=9 -> 5 floors:  7  8  9 10 11
  // z=10 -> 5 floors:  8  9 10 11 12
  // z=11 -> 5 floors:  9 10 11 12 13
  // z=12 -> 5 floors: 10 11 12 13 14
  // z=13 -> 5 floors: 11 12 13 14 15
  // z=14 -> 4 floors: 12 13 14 15
  // z=15 -> 3 floors: 13 14 15

  auto skip = 0;
  for (auto z = z_start; z != (z_end + z_step); z += z_step)
  {
    for (auto x = 0; x < width; x++)
    {
      for (auto y = 0; y < height; y++)
      {
        protocol::Tile tile = {};
        if (skip > 0)
        {
          skip -= 1;
          tile.skip = true;
          tiles.push_back(std::move(tile));
          continue;
        }

        // Parse tile
        tile.skip = false;
        for (auto stackpos = 0; true; stackpos++)
        {
          if (packet->peekU16() >= 0xFF00)
          {
            skip = packet->getU16() & 0xFF;
            break;
          }

          if (stackpos > 10)
          {
            LOG_ERROR("%s: too many things on this tile", __func__);
          }

          tile.things.emplace_back(protocol::getThing(packet));
        }

        tiles.push_back(std::move(tile));
      }
    }
  }

  return tiles;
}

}  // namespace

namespace protocol::client
{

Login getLogin(network::IncomingPacket* packet)
{
  Login login;
  packet->get(&login.player_id);
  packet->get(&login.server_beat);
  packet->getU8();  // canReportBugs
  return login;
}

LoginFailed getLoginFailed(network::IncomingPacket* packet)
{
  LoginFailed failed;
  packet->get(&failed.reason);
  return failed;
}

Equipment getEquipment(bool empty, network::IncomingPacket* packet)
{
  Equipment equipment;
  equipment.empty = empty;
  packet->get(&equipment.inventory_index);
  if (equipment.empty)
  {
    equipment.item = getItem(packet);
  }
  return equipment;
}

MagicEffect getMagicEffect(network::IncomingPacket* packet)
{
  MagicEffect effect;
  effect.position = getPosition(packet);
  packet->get(&effect.type);
  return effect;
}

PlayerStats getPlayerStats(network::IncomingPacket* packet)
{
  PlayerStats stats;
  packet->get(&stats.health);
  packet->get(&stats.max_health);
  packet->get(&stats.capacity);
  packet->get(&stats.exp);
  packet->get(&stats.level);
  packet->get(&stats.level_perc);
  packet->get(&stats.mana);
  packet->get(&stats.max_mana);
  packet->get(&stats.magic_level);
  packet->get(&stats.magic_level_perc);
  return stats;
}

WorldLight getWorldLight(network::IncomingPacket* packet)
{
  WorldLight light;
  packet->get(&light.intensity);
  packet->get(&light.color);
  return light;
}

PlayerSkills getPlayerSkills(network::IncomingPacket* packet)
{
  PlayerSkills skills;
  packet->get(&skills.fist);
  packet->get(&skills.fist_perc);
  packet->get(&skills.club);
  packet->get(&skills.club_perc);
  packet->get(&skills.sword);
  packet->get(&skills.sword_perc);
  packet->get(&skills.axe);
  packet->get(&skills.axe_perc);
  packet->get(&skills.dist);
  packet->get(&skills.dist_perc);
  packet->get(&skills.shield);
  packet->get(&skills.shield_perc);
  packet->get(&skills.fish);
  packet->get(&skills.fish_perc);
  return skills;
}

TextMessage getTextMessage(network::IncomingPacket* packet)
{
  TextMessage message;
  packet->get(&message.type);
  packet->get(&message.message);
  return message;
}

ThingAdded getThingAdded(network::IncomingPacket* packet)
{
  ThingAdded thing_added;
  thing_added.position = getPosition(packet);
  thing_added.thing = getThing(packet);
  return thing_added;
}

ThingChanged getThingChanged(network::IncomingPacket* packet)
{
  ThingChanged thing_changed;
  thing_changed.position = getPosition(packet);
  packet->get(&thing_changed.stackpos);
  thing_changed.thing = getThing(packet);
  return thing_changed;
}

ThingRemoved getThingRemoved(network::IncomingPacket* packet)
{
  ThingRemoved thing_removed;
  (void)packet;
  return thing_removed;
}

ThingMoved getThingMoved(network::IncomingPacket* packet)
{
  ThingMoved thing_moved;
  thing_moved.old_position = getPosition(packet);
  packet->get(&thing_moved.old_stackpos);
  thing_moved.new_position = getPosition(packet);
  return thing_moved;
}

FullMap getFullMap(network::IncomingPacket* packet)
{
  FullMap map = {};
  map.position = getPosition(packet);
  map.tiles = getMapData(map.position.getZ(), 18, 14, packet);
  return map;
}

PartialMap getPartialMap(int z, common::Direction direction, network::IncomingPacket* packet)
{
  PartialMap map = {};
  map.direction = direction;
  switch (direction)
  {
    case common::Direction::NORTH:
    case common::Direction::SOUTH:
      map.tiles = getMapData(z, 18, 1, packet);
      break;

    case common::Direction::EAST:
    case common::Direction::WEST:
      map.tiles = getMapData(z, 1, 14, packet);
      break;
  }
  return map;
}

}  // namespace protocol::client
