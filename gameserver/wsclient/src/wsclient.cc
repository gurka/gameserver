#include <cstdint>
#include <string>
#include <vector>

#include "logger.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"
#include "position.h"
#include "protocol_helper.h"
#include "protocol_types.h"

#include "network.h"

std::uint32_t playerId;
ProtocolTypes::MapData mapData;

void handleLoginPacket(const ProtocolTypes::Login& login)
{
  playerId = login.playerId;
}

void handleLoginFailedPacket(const ProtocolTypes::LoginFailed& failed)
{
  LOG_ERROR("Could not login: %s", failed.reason.c_str());
}

void handleFullMapPacket(const ProtocolTypes::MapData& map)
{
  mapData = map;
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
  Network::start(&handle_packet);
}
