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

#include "protocol.h"

#include "game/game.h"
#include "chat/chat.h"
#include "network/incoming_packet.h"
#include "protocol/protocol_client.h"
#include "utils/logger.h"

Protocol::Protocol(game::Game* game, chat::Chat* chat, sidebar::Sidebar* sidebar)
    : m_num_handled_packets(0),
      m_game(game),
      m_chat(chat),
      m_sidebar(sidebar)
{
}

void Protocol::handlePacket(network::IncomingPacket* packet)
{
  ++m_num_handled_packets;

  LOG_INFO("%s: handling packet number %d", __func__, m_num_handled_packets);

  while (!packet->isEmpty())
  {
    const auto type = packet->getU8();
    LOG_DEBUG("%s: type: 0x%02X", __func__, type);
    switch (type)
    {
      case 0x0A:
        handleLoginPacket(protocol::client::getLogin(packet));
        break;

      case 0x0B:
      {
        // handleGMActions?
        for (auto i = 0; i < 32; ++i)
        {
          packet->getU8();
        }
        break;
      }

      case 0x14:
        handleLoginFailedPacket(protocol::client::getLoginFailed(packet));
        break;

      case 0x64:
        handleFullMapPacket(protocol::client::getFullMap(packet));
        break;

      case 0x65:
      case 0x66:
      case 0x67:
      case 0x68:
        handlePartialMapPacket(protocol::client::getPartialMap(m_game->getPlayerPosition().getZ(),
                                                               static_cast<common::Direction>(type - 0x65),
                                                               packet));
        break;

      case 0x69:
        handleTileUpdatePacket(protocol::client::getTileUpdate(packet));
        break;

      case 0x6A:
        handleThingAdded(protocol::client::getThingAdded(packet));
        break;

      case 0x6B:
        handleThingChanged(protocol::client::getThingChanged(packet));
        break;

      case 0x6D:
        handleThingMoved(protocol::client::getThingMoved(packet));
        break;

      case 0x6C:
        handleThingRemoved(protocol::client::getThingRemoved(packet));
        break;

      case 0x83:
        handleMagicEffect(protocol::client::getMagicEffect(packet));
        break;

      case 0x84:
      {
        // animatedText
        protocol::getPosition(packet);
        packet->getU8();  // color
        const auto message = packet->getString();  // text

        // TODO, to Game
        break;
      }
      case 0xA0:
        handlePlayerStats(protocol::client::getPlayerStats(packet));
        break;

      case 0x82:
        handleWorldLight(protocol::client::getWorldLight(packet));
        break;

      case 0xA1:
        handlePlayerSkills(protocol::client::getPlayerSkills(packet));
        break;

      case 0xAC:
      {
        // open channel
        const auto id = packet->getU16();
        const auto name = packet->getString();
        m_chat->openChannel(id, name);
        break;
      }

      case 0x6F:
      {
        // close container
        packet->getU8();  // cid
        break;
      }

      case 0x70:
      {
        // container add item
        packet->getU8();  // cid
        protocol::getItem(packet);  // item (container item?)
        break;
      }

      case 0x71:
      {
        // container update item
        packet->getU8();  // cid
        packet->getU8();  // slot
        protocol::getItem(packet);
        break;
      }

      case 0x72:
      {
        // container remove item
        packet->getU8();  // cid
        packet->getU8();  // slot
        break;
      }

      case 0x78:
      case 0x79:
        handleEquipmentUpdate(protocol::client::getEquipment(type == 0x79, packet));
        break;

      case 0xB4:
        handleTextMessage(protocol::client::getTextMessage(packet));
        break;

      case 0x8C:
        // update creature health
        packet->getU32();  // creature id
        packet->getU8();  // health perc
        break;

      case 0x8D:
        // creature light
        packet->getU32();  // creature id
        packet->getU8(); // light intensity
        packet->getU8(); // light color
        break;

      case 0xD2:
        // add name to VIP list
        packet->getU32();  // id
        packet->getString();  // name
        packet->getU8();  // status
        break;

      case 0x6E:
      {
        // open container
        packet->getU8();  // container id
        protocol::getItem(packet);  // container item
        packet->getString();  // container name
        packet->getU8();  // capacity / slots
        packet->getU8();  // 0 = no parent, else has parent
        auto num_items = packet->getU8();
        while (num_items-- > 0)
        {
          protocol::getItem(packet);
        }
        break;
      }

      case 0xAA:
      {
        // talk
        const auto talker = packet->getString();  // talker
        const auto talk_type = packet->getU8();  // type
        switch (talk_type)
        {
          case 1:  // say
          case 2:  // whisper
          case 3:  // yell
          case 16:  // monster?
          case 17:  // monster?
          {
            // Both Game and Chat needs to know
            const auto position =  protocol::getPosition(packet);
            (void)position;
            const auto text = packet->getString();  // text
            // TODO Game
            m_chat->message(talker, talk_type, text);
            break;
          }

          case 5:   // channel
          case 10:  // gm?
          case 14:  // ??
          {
            const auto channel_id = packet->getU16();  // channel id?
            const auto text = packet->getString();  // text
            m_chat->message(talker, talk_type, channel_id, text);
            break;
          }

          case 4:  // whisper?
          {
            const auto text = packet->getString();  // text
            m_chat->message(talker, talk_type, text);
            break;
          }

          default:
            LOG_ERROR("%s: unknown talk type: %u", __func__, talk_type);
            break;
        }
        break;
      }

      case 0xAD:
        // Open private channel
        m_chat->openPrivateChannel(packet->getString());
        break;

      case 0xB5:
        // cancel walk
        packet->getU8();  // dir -> change player to this dir
        break;

      case 0xA2:
        // player state
        packet->getU8();
        break;

      case 0x8F:
        // creature speed
        packet->getU32();  // creature id
        packet->getU16();  // new speed
        break;

      case 0xBE:
      case 0xBF:
      {
        const auto up = type == 0xBE;
        // Moved up from underground to sea level:     read 6 floors
        // Moved up from underground to underground:   read 1 floor
        // Moved down from sea level to underground:   read 3 floors
        // Moved down from underground to underground: read 1 floor, unless we are on z=14 or z=15 then read 0 floors
        // Moved up/down from sea level to sea level:  read 0 floors
        const auto num_floors = (( up && m_game->getPlayerPosition().getZ() == 8) ? 6 :
                                 (( up && m_game->getPlayerPosition().getZ()  > 8) ? 1 :
                                  ((!up && m_game->getPlayerPosition().getZ() == 7) ? 3 :
                                   ((!up && m_game->getPlayerPosition().getZ()  > 7 && m_game->getPlayerPosition().getZ() < 13) ? 1 : (0)))));
        handleFloorChange(up, protocol::client::getFloorChange(num_floors,
                                                               game::KNOWN_TILES_X,
                                                               game::KNOWN_TILES_Y,
                                                               packet));
        break;
      }

      case 0xA3:
        // cancel attack
        break;

      case 0x85:
        // missile
        protocol::getPosition(packet);  // from
        protocol::getPosition(packet);  // to
        packet->getU8();  // missile id
        break;

      case 0x90:
        handleCreatureSkull(protocol::client::getCreatureSkull(packet));
        break;

      case 0x86:
        // mark creature
        packet->getU32();  // creature id
        packet->getU8();  // color
        // show for 1000ms?
        break;

      case 0xD4:
        // vip logout
        packet->getU32();  // vip id
        break;

      case 0x91:
        // player shield icon
        packet->getU32();  // creature id
        packet->getU8();  // shield icon
        break;

      case 0x1E:
        // ping
        break;

      default:
        LOG_ABORT("%s: unknown packet type: 0x%X at position %u (position %u with packet header) num recv packets: %d",
                  __func__,
                  type,
                  packet->getPosition() - 1,
                  packet->getPosition() + 1,
                  m_num_handled_packets);
    }
  }
}

void Protocol::handleLoginPacket(const protocol::client::Login& login)
{
  m_game->setPlayerId(login.player_id);
}

void Protocol::handleLoginFailedPacket(const protocol::client::LoginFailed& failed)
{
  LOG_ERROR("Could not login: %s", failed.reason.c_str());
}

void Protocol::handleFullMapPacket(const protocol::client::FullMap& map_data)
{
  m_game->setFullMapData(map_data);
}

void Protocol::handlePartialMapPacket(const protocol::client::PartialMap& map_data)
{
  m_game->setPartialMapData(map_data);
}

void Protocol::handleTileUpdatePacket(const protocol::client::TileUpdate& tile_update)
{
  m_game->updateTile(tile_update);
}

void Protocol::handleFloorChange(bool up, const protocol::client::FloorChange& floor_change)
{
  m_game->handleFloorChange(up, floor_change);
}

void Protocol::handleMagicEffect(const protocol::client::MagicEffect& effect)
{
  (void)effect;
}

void Protocol::handlePlayerStats(const protocol::client::PlayerStats& stats)
{
  // TODO
  //player_info.stats = stats;
}

void Protocol::handleWorldLight(const protocol::client::WorldLight& light)
{
  (void)light;
}

void Protocol::handlePlayerSkills(const protocol::client::PlayerSkills& skills)
{
  // TODO
  //player_info.skills = skills;
}

void Protocol::handleEquipmentUpdate(const protocol::client::Equipment& equipment)
{
  // TODO
  /*
  player_info.equipment[equipment.inventory_index - 1] =
      {
          equipment.empty ? nullptr : &itemtypes[equipment.item.item_type_id],
          static_cast<std::uint8_t>(equipment.empty ? 0U : equipment.item.extra)
      };
  */
}

void Protocol::handleTextMessage(const protocol::client::TextMessage& message)
{
  // TODO
  //text_messages.push_back(message.message);
}

void Protocol::handleThingAdded(const protocol::client::ThingAdded& thing_added)
{
  m_game->addProtocolThing(thing_added.position, thing_added.thing);
}

void Protocol::handleThingChanged(const protocol::client::ThingChanged& thing_changed)
{
  m_game->updateThing(thing_changed.position, thing_changed.stackpos, thing_changed.thing);
}

void Protocol::handleThingMoved(const protocol::client::ThingMoved& thing_moved)
{
  m_game->moveThing(thing_moved.old_position, thing_moved.old_stackpos, thing_moved.new_position);
}

void Protocol::handleThingRemoved(const protocol::client::ThingRemoved& thing_removed)
{
  m_game->removeThing(thing_removed.position, thing_removed.stackpos);
}

void Protocol::handleCreatureSkull(const protocol::client::CreatureSkull& creature_skull)
{
  m_game->setCreatureSkull(creature_skull.creature_id, creature_skull.skull);
}
