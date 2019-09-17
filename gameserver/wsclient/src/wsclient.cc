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
#include <cstdint>
#include <string>
#include <vector>

#include "logger.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"
#include "position.h"
#include "protocol.h"
#include "protocol_types.h"

#include "graphics.h"
#include "item_types.h"
#include "map.h"
#include "network.h"
#include "types.h"

namespace wsclient
{

using namespace protocol::client;

std::uint32_t player_id;
world::Position player_position = { 0, 0, 0 };
world::Map map;
std::vector<Creature> creatures;

void handleLoginPacket(const Login& login)
{
  player_id = login.player_id;
}

void handleLoginFailedPacket(const LoginFailed& failed)
{
  LOG_ERROR("Could not login: %s", failed.reason.c_str());
}

void handleFullMapPacket(const MapData& mapData)
{
  player_position = mapData.position;
  map.setMapData(mapData);
  graphics::draw(map, player_position, player_id);
}

void handleMagicEffect(const MagicEffect& effect)
{
}

void handlePlayerStats(const PlayerStats& stats)
{
}

void handleWorldLight(const WorldLight& light)
{
}

void handlePlayerSkills(const PlayerSkills& skills)
{
}

void handleEquipmentUpdate(const Equipment& equipment)
{
}

void handleTextMessage(const TextMessage& message)
{
  LOG_INFO("%s: message: %s", __func__, message.message.c_str());
}

void handleCreatureMove(const CreatureMove& move)
{
  LOG_INFO("%s: can_see_old_pos: %s, can_see_new_pos: %s",
           __func__,
           move.can_see_old_pos ? "true" : "false",
           move.can_see_new_pos ? "true" : "false");

  if (move.can_see_old_pos && move.can_see_new_pos)
  {
    // Move creature
    const auto cid = map.getTile(move.old_position).things[move.old_stackpos].creature_id;
    map.removeThing(move.old_position, move.old_stackpos);
    map.addCreature(move.new_position, cid);
  }
  else if (move.can_see_old_pos)
  {
    // Remove creature
    map.removeThing(move.old_position, move.old_stackpos);
  }
  else if (move.can_see_new_pos)
  {
    // Add new creature
    map.addCreature(move.new_position, move.creature.id);
  }

  // If this played moved then we need to update map's player_position
  // BEFORE OR AFTER MOVING PLAYER???

  graphics::draw(map, player_position, player_id);
}

void handle_packet(network::IncomingPacket* packet)
{
  while (!packet->isEmpty())
  {
    const auto type = packet->getU8();
    switch (type)
    {
      case 0x0A:
        handleLoginPacket(getLogin(packet));
        break;

      case 0x14:
        handleLoginFailedPacket(getLoginFailed(packet));
        break;

      case 0x64:
        handleFullMapPacket(getMapData(18, 14, packet));
        break;

      case 0x83:
        handleMagicEffect(getMagicEffect(packet));
        break;

      case 0xA0:
        handlePlayerStats(getPlayerStats(packet));
        break;

      case 0x82:
        handleWorldLight(getWorldLight(packet));
        break;

      case 0xA1:
        handlePlayerSkills(getPlayerSkills(packet));
        break;

      case 0x78:
      case 0x79:
        handleEquipmentUpdate(getEquipment(type == 0x78, packet));
        break;

      case 0xB4:
        handleTextMessage(getTextMessage(packet));
        break;

      case 0x6C:
      case 0x6D:
      case 0x6A:
        handleCreatureMove(getCreatureMove(type == 0x6D || type == 0x6C,
                                           type == 0x6D || type == 0x6A,
                                           packet));
        break;

      default:
        LOG_ERROR("%s: unknown packet type: 0x%X", __func__, type);
        return;
    }
  }
}

}  // namespace wsclient

int main()
{
  const auto item_types = wsclient::item_types::load("data.dat");
  if (!item_types)
  {
    LOG_ERROR("Could not load \"data.dat\"");
    return 1;
  }

  wsclient::graphics::init(&(item_types.value()));
  wsclient::network::start(&wsclient::handle_packet);
}
