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

std::uint32_t playerId;
Position playerPosition;
types::Map map;

void handleLoginPacket(const ProtocolTypes::Login& login)
{
  playerId = login.playerId;
}

void handleLoginFailedPacket(const ProtocolTypes::LoginFailed& failed)
{
  LOG_ERROR("Could not login: %s", failed.reason.c_str());
}

void handleFullMapPacket(const ProtocolTypes::MapData& mapData)
{
  playerPosition = mapData.position;
  auto it = mapData.tiles.begin();
  for (auto x = 0; x < types::known_tiles_x; x++)
  {
    for (auto y = 0; y < types::known_tiles_y; y++)
    {
      if (it->skip)
      {
        map[y][x] = ProtocolTypes::MapData::TileData();
      }
      else
      {
        map[y][x] = *it;
      }
      ++it;
    }
  }

  Graphics::draw(map, playerPosition);
}

void handleMagicEffect(const ProtocolTypes::MagicEffect& effect)
{
}

void handlePlayerStats(const ProtocolTypes::PlayerStats& stats)
{
}

void handleWorldLight(const ProtocolTypes::WorldLight& light)
{
}

void handlePlayerSkills(const ProtocolTypes::PlayerSkills& skills)
{
}

void handleEquipmentUpdate(const ProtocolTypes::Equipment& equipment)
{
}

void handleTextMessage(const ProtocolTypes::TextMessage& message)
{
  LOG_INFO("%s: message: %s", __func__, message.message.c_str());
}

void handle_packet(IncomingPacket* packet)
{
  using namespace ProtocolHelper;

  LOG_INFO("%s", __func__);
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
