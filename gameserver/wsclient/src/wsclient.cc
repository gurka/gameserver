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
#include "protocol_helper.h"
#include "protocol_types.h"

#include "types.h"
#include "network.h"
#include "graphics.h"
#include "map.h"

std::uint32_t playerId;
Position playerPosition;
Map map;
std::vector<ProtocolTypes::Client::Creature> creatures;

void handleLoginPacket(const ProtocolTypes::Client::Login& login)
{
  playerId = login.playerId;
}

void handleLoginFailedPacket(const ProtocolTypes::Client::LoginFailed& failed)
{
  LOG_ERROR("Could not login: %s", failed.reason.c_str());
}

void handleFullMapPacket(const ProtocolTypes::Client::MapData& mapData)
{
  playerPosition = mapData.position;
  map.setMapData(mapData);
  Graphics::draw(map, playerPosition, playerId);
}

void handleMagicEffect(const ProtocolTypes::Client::MagicEffect& effect)
{
}

void handlePlayerStats(const ProtocolTypes::Client::PlayerStats& stats)
{
}

void handleWorldLight(const ProtocolTypes::Client::WorldLight& light)
{
}

void handlePlayerSkills(const ProtocolTypes::Client::PlayerSkills& skills)
{
}

void handleEquipmentUpdate(const ProtocolTypes::Client::Equipment& equipment)
{
}

void handleTextMessage(const ProtocolTypes::Client::TextMessage& message)
{
  LOG_INFO("%s: message: %s", __func__, message.message.c_str());
}

void handleCreatureMove(const ProtocolTypes::Client::CreatureMove& move)
{
  LOG_INFO("%s: canSeeOldPos: %s, canSeeNewPos: %s",
           __func__,
           move.canSeeOldPos ? "true" : "false",
           move.canSeeNewPos ? "true" : "false");

  if (move.canSeeOldPos && move.canSeeNewPos)
  {
    // Move creature
    const auto cid = map.getTile(move.oldPosition).things[move.oldStackPosition].creatureId;
    map.removeThing(move.oldPosition, move.oldStackPosition);
    map.addCreature(move.newPosition, cid);
  }
  else if (move.canSeeOldPos)
  {
    // Remove creature
    map.removeThing(move.oldPosition, move.oldStackPosition);
  }
  else if (move.canSeeNewPos)
  {
    // Add new creature
    map.addCreature(move.newPosition, move.creature.id);
  }

  // If this played moved then we need to update map's playerPosition
  // BEFORE OR AFTER MOVING PLAYER???

  Graphics::draw(map, playerPosition, playerId);
}

void handle_packet(IncomingPacket* packet)
{
  using namespace ProtocolHelper::Client;
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

int main()
{
  Graphics::init();
  Network::start(&handle_packet);
}
