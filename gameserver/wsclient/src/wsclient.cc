#include <cstdint>
#include <string>
#include <vector>

#include "logger.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"
#include "position.h"

#include "network.h"

std::uint32_t playerId;

Position getPosition(IncomingPacket* packet)
{
  const auto x = packet->getU16();
  const auto y = packet->getU16();
  const auto z = packet->getU8();
  return Position(x, y, z);
}

void getItem(IncomingPacket* packet)
{
  packet->getU16();  // ItemTypeId
  // assume not stackable or multitype
}

void getMapData(IncomingPacket* packet, int width, int height)
{
  // Assume that we always are on z=7
  auto skip = 0;
  for (auto z = 7; z >= 0; z--)
  {
    for (auto x = 0; x < width; x++)
    {
      for (auto y = 0; y < height; y++)
      {
        if (skip > 0)
        {
          skip -= 1;
          continue;
        }

        // Parse tile
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
            return;
          }

          if (packet->peekU16() == 0x0061 ||
              packet->peekU16() == 0x0062)
          {
            std::uint32_t creatureId;
            std::string creatureName;
            if (packet->getU16() == 0x0061)
            {
              // New creature
              packet->getU32();  // creatureId to remove
              creatureId = packet->getU32();
              creatureName = packet->getString();
            }
            else  // 0x0062
            {
              // Known creature
              creatureId = packet->getU32();
            }

            LOG_INFO("%s: parsed creature with id: %d and name: %s",
                     __func__,
                     creatureId,
                     creatureName.c_str());

            packet->getU8();  // hp %
            packet->getU8();  // direction
            packet->getU8();  // outfit
            packet->getU8();
            packet->getU8();
            packet->getU8();
            packet->getU8();
            packet->getU16();  // unknown 0x00 0xDC
            packet->getU16();  // speed
          }
          else
          {
            getItem(packet);
          }
        }
      }
    }
  }
}

void handleLoginPacket(IncomingPacket* packet)
{
  playerId = packet->getU32();
  packet->getU16();  // server beat
}

void handleLoginFailedPacket(IncomingPacket* packet)
{
  const auto reason = packet->getString();
  LOG_ERROR("Could not login: %s", reason.c_str());
}

void handleFullMapPacket(IncomingPacket* packet)
{
  getPosition(packet);
  getMapData(packet, 18, 14);
}

void handleMagicEffect(IncomingPacket* packet)
{
  getPosition(packet);
  packet->getU8();  // 
}

void handlePlayerStats(IncomingPacket* packet)
{
  packet->getU16();  // hp
  packet->getU16();  // max hp
  packet->getU16();  // cap
  packet->getU32();  // exp
  packet->getU8();   // level
  packet->getU16();  // mana
  packet->getU16();  // max mana
  packet->getU8();   // magic level
}

void handleWorldLight(IncomingPacket* packet)
{
  packet->getU8();  // intensity
  packet->getU8();  // color
}

void handlePlayerSkills(IncomingPacket* packet)
{
  for (auto i = 0; i < 7; i++)
  {
    packet->getU8();
  }
}

void handleEquipmentUpdate(IncomingPacket* packet, bool empty)
{
  packet->getU8();  // index
  if (!empty)
  {
    getItem(packet);
  }
}

void handleTextMessage(IncomingPacket* packet)
{
  packet->getU8();  // type
  const auto message = packet->getString();
  LOG_INFO("%s: message: %s", __func__, message.c_str());
}

void handle_packet(IncomingPacket* packet)
{
  LOG_INFO("%s", __func__);
  while (!packet->isEmpty())
  {
    const auto type = packet->getU8();
    switch (type)
    {
      case 0x0A:
        handleLoginPacket(packet);
        break;

      case 0x14:
        handleLoginFailedPacket(packet);
        break;

      case 0x64:
        handleFullMapPacket(packet);
        break;

      case 0x83:
        handleMagicEffect(packet);
        break;

      case 0xA0:
        handlePlayerStats(packet);
        break;

      case 0x82:
        handleWorldLight(packet);
        break;

      case 0xA1:
        handlePlayerSkills(packet);
        break;

      case 0x78:
      case 0x79:
        handleEquipmentUpdate(packet, type == 0x78);
        break;

      case 0xB4:
        handleTextMessage(packet);
        break;

      default:
        LOG_ERROR("%s: unknown packet type: 0x%X", __func__, type);
        return;
    }
  }
}

int main()
{
  Network::start(&handle_packet);
}
