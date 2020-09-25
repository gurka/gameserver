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

#include <cstdint>
#include <string>
#include <vector>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/val.h>
#include <SDL.h>
#else
#include <asio.hpp>
#include <SDL2/SDL.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

// network
#include "client_factory.h"
#include "connection.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"

// protocol
#include "protocol_common.h"
#include "protocol_client.h"

// utils
#include "data_loader.h"
#include "logger.h"

// common
#include "position.h"

// wsclient
#include "graphics.h"
#include "wsworld.h"
#include "types.h"

namespace wsclient
{

std::unique_ptr<network::Connection> connection;
utils::data_loader::ItemTypes itemtypes;
wsworld::Map map;
int num_received_packets = 0;

void handleLoginPacket(const protocol::client::Login& login)
{
  map.setPlayerId(login.player_id);
}

void handleLoginFailedPacket(const protocol::client::LoginFailed& failed)
{
  LOG_ERROR("Could not login: %s", failed.reason.c_str());
}

void handleFullMapPacket(const protocol::client::FullMap& map_data)
{
  map.setFullMapData(map_data);
}

void handlePartialMapPacket(const protocol::client::PartialMap& map_data)
{
  map.setPartialMapData(map_data);
}

void handleMagicEffect(const protocol::client::MagicEffect& effect)
{
  (void)effect;
}

void handlePlayerStats(const protocol::client::PlayerStats& stats)
{
  (void)stats;
}

void handleWorldLight(const protocol::client::WorldLight& light)
{
  (void)light;
}

void handlePlayerSkills(const protocol::client::PlayerSkills& skills)
{
  (void)skills;
}

void handleEquipmentUpdate(const protocol::client::Equipment& equipment)
{
  (void)equipment;
}

void handleTextMessage(const protocol::client::TextMessage& message)
{
  LOG_INFO("%s: message: %s", __func__, message.message.c_str());
}

void handleThingAdded(const protocol::client::ThingAdded& thing_added)
{
  map.addProtocolThing(thing_added.position, thing_added.thing);
}

void handleThingChanged(const protocol::client::ThingChanged thing_changed)
{
  map.updateThing(thing_changed.position, thing_changed.stackpos, thing_changed.thing);
}

void handleThingMoved(const protocol::client::ThingMoved& thing_moved)
{
  map.moveThing(thing_moved.old_position, thing_moved.old_stackpos, thing_moved.new_position);
}

void handlePacket(network::IncomingPacket* packet)
{
  ++num_received_packets;

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
        handlePartialMapPacket(protocol::client::getPartialMap(map.getPlayerPosition().getZ(),
                                                               static_cast<common::Direction>(type - 0x65),
                                                               packet));
        break;

      case 0x6A:
        handleThingAdded(protocol::client::getThingAdded(packet));
        break;

      case 0x6B:
      {
        handleThingChanged(protocol::client::getThingChanged(packet));
        break;
      }

      case 0x6D:
        handleThingMoved(protocol::client::getThingMoved(packet));
        break;

      case 0x83:
        handleMagicEffect(protocol::client::getMagicEffect(packet));
        break;

      case 0x84:
      {
        // animatedText
        protocol::getPosition(packet);
        packet->getU8();  // color
        packet->getString();  // text
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
        LOG_INFO("%s: open channel %u -> %s", __func__, id, name.c_str());
        break;
      }

      case 0x70:
      {
        // container add item
        packet->getU8();  // cid
        protocol::getItem(packet);  // item (container item?)
        break;
      }

      case 0x78:
      case 0x79:
        handleEquipmentUpdate(protocol::client::getEquipment(type == 0x78, packet));
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
        packet->getString();  // talker
        const auto type = packet->getU8();  // type
        switch (type)
        {
          case 1:  // say
          case 2:  // whisper
          case 3:  // yell
          case 16:  // monster?
          case 17:  // monster?
            protocol::getPosition(packet);
            break;

          case 5:   // channel
          case 10:  // gm?
          case 14:  // ??
            packet->getU16();  // channel id?
            break;

          default:
            LOG_ERROR("%s: unknown talk type: %u", __func__, type);
            break;
        }
        packet->getString();  // text
        break;
      }

      default:
        LOG_ERROR("%s: unknown packet type: 0x%X at position %u (position %u with packet header) num recv packets: %d",
                  __func__,
                  type,
                  packet->getPosition() - 1,
                  packet->getPosition() + 1,
                  num_received_packets);
        return;
    }
  }
}

void sendMoveCharacter(common::Direction direction)
{
  if (connection)
  {
    network::OutgoingPacket packet;
    packet.addU8(static_cast<std::uint8_t>(direction) + 0x65);
    connection->sendPacket(std::move(packet));
  }
}

}  // namespace wsclient

#ifndef EMSCRIPTEN
static asio::io_context io_context;
static bool stop = false;
static std::function<void(void)> main_loop_func;
static std::unique_ptr<asio::deadline_timer> timer;

void timerCallback(const asio::error_code& ec)
{
  if (ec)
  {
    LOG_ERROR("%s: ec: %s", __func__, ec.message().c_str());
    stop = true;
    return;
  }

  if (stop)
  {
    LOG_INFO("%s: stop=true", __func__);
    return;
  }

  main_loop_func();
  timer->expires_from_now(boost::posix_time::millisec(16));
  timer->async_wait(&timerCallback);
}

void emscripten_set_main_loop(std::function<void(void)> func, int fps, int loop)  // NOLINT
{
  (void)fps;
  (void)loop;

  main_loop_func = std::move(func);
  timer = std::make_unique<asio::deadline_timer>(io_context);
  timer->expires_from_now(boost::posix_time::millisec(16));
  timer->async_wait(&timerCallback);
  io_context.run();
}

void emscripten_cancel_main_loop()  // NOLINT
{
  if (!stop)
  {
    stop = true;
    timer->cancel();
  }
}
#endif

extern "C" void main_loop()  // NOLINT
{
  // Read input
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0)
  {
    if (event.type == SDL_KEYDOWN)
    {
      switch (event.key.keysym.scancode)
      {
        case SDL_SCANCODE_LEFT:
          wsclient::sendMoveCharacter(common::Direction::WEST);
          break;

        case SDL_SCANCODE_RIGHT:
          wsclient::sendMoveCharacter(common::Direction::EAST);
          break;

        case SDL_SCANCODE_UP:
          wsclient::sendMoveCharacter(common::Direction::NORTH);
          break;

        case SDL_SCANCODE_DOWN:
          wsclient::sendMoveCharacter(common::Direction::SOUTH);
          break;

        case SDL_SCANCODE_ESCAPE:
          if (wsclient::connection)
          {
            LOG_INFO("%s: closing connection", __func__);
            wsclient::connection->close(true);
          }
          LOG_INFO("%s: stopping client", __func__);
          emscripten_cancel_main_loop();
          return;

        default:
          break;
      }
    }
  }

  // Render
  wsclient::graphics::draw(wsclient::map);
}

int main()
{
  utils::Logger::setLevel("network", utils::Logger::Level::INFO);

  constexpr auto data_filename = "files/data.dat";
  constexpr auto sprite_filename = "files/sprite.dat";

  if (!utils::data_loader::load(data_filename, &wsclient::itemtypes, nullptr, nullptr))
  {
    LOG_ERROR("%s: could not load data file: %s", __func__, data_filename);
    return 1;
  }

  protocol::setItemTypes(&wsclient::itemtypes);
  wsclient::map.setItemTypes(&wsclient::itemtypes);
  if (!wsclient::graphics::init(&wsclient::itemtypes, sprite_filename))
  {
    LOG_ERROR("%s: could not initialize graphics", __func__);
    return 1;
  }

#ifdef EMSCRIPTEN
  const auto usp = emscripten::val::global("URLSearchParams").new_(emscripten::val::global("location")["search"]);
  const auto uri = usp.call<bool>("has", emscripten::val("uri")) ?
                   usp.call<std::string>("get", emscripten::val("uri")) :
                   "ws://localhost:8172";
  LOG_INFO("%s: found uri: '%s'", __func__, uri.c_str());
#else
  const auto uri = std::string("ws://localhost:8172");
#endif

  network::ClientFactory::Callbacks callbacks =
  {
      [](std::unique_ptr<network::Connection>&& connection)
      {
        LOG_INFO("%s: connected", __func__);
        wsclient::connection = std::move(connection);

        network::Connection::Callbacks connection_callbacks =
        {
          &wsclient::handlePacket,
          []()
          {
            LOG_INFO("%s: disconnected", __func__);
            wsclient::connection.reset();
          }
        };
        wsclient::connection->init(connection_callbacks, false);
      },
      []() { LOG_INFO("%s: could not connect", __func__); },
  };
#ifdef EMSCRIPTEN
  if (!network::ClientFactory::createWebsocketClient(uri, callbacks))
#else
  if (!network::ClientFactory::createWebsocketClient(&io_context, uri, callbacks))
#endif
  {
    LOG_ERROR("%s: could not create connection", __func__);
  }

  LOG_INFO("%s: starting main loop", __func__);
  emscripten_set_main_loop(main_loop, 0, 1);
}
