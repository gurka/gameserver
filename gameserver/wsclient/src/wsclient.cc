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

#include <emscripten.h>
#include <emscripten/val.h>
#include <SDL.h>

#include "logger.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"
#include "position.h"
#include "protocol_common.h"
#include "protocol_client.h"
#include "data_loader.h"

#include "graphics.h"
#include "wsworld.h"
#include "network.h"
#include "types.h"

namespace wsclient
{

using namespace protocol::client;

io::data_loader::ItemTypes itemtypes;
wsworld::Map map;

void handleLoginPacket(const Login& login)
{
  map.setPlayerId(login.player_id);
}

void handleLoginFailedPacket(const LoginFailed& failed)
{
  LOG_ERROR("Could not login: %s", failed.reason.c_str());
}

void handleFullMapPacket(const FullMap& map_data)
{
  map.setFullMapData(map_data);
}

void handlePartialMapPacket(const PartialMap& map_data)
{
  map.setPartialMapData(map_data);
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

void handleThingAdded(const ThingAdded& thing_added)
{
  map.addProtocolThing(thing_added.position, thing_added.thing);
}

void handleThingMoved(const ThingMoved& thing_moved)
{
  map.moveThing(thing_moved.old_position, thing_moved.old_stackpos, thing_moved.new_position);
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
        handleFullMapPacket(getFullMap(packet));
        break;

      case 0x65:
      case 0x66:
      case 0x67:
      case 0x68:
        handlePartialMapPacket(getPartialMap(static_cast<common::Direction>(type - 0x65), packet));
        break;

      case 0x6A:
        handleThingAdded(getThingAdded(packet));
        break;

      case 0x6D:
        handleThingMoved(getThingMoved(packet));
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

void sendMoveCharacter(common::Direction direction)
{
  network::OutgoingPacket packet;
  packet.addU8(static_cast<std::uint8_t>(direction) + 0x65);
  network::sendPacket(std::move(packet));
}

}  // namespace wsclient

extern "C" void main_loop()
{
  // Read input
  SDL_Event event;
  while (SDL_PollEvent(&event))
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
  constexpr auto data_filename = "files/data.dat";
  constexpr auto sprite_filename = "files/sprite.dat";

  if (!io::data_loader::load(data_filename, &wsclient::itemtypes, nullptr, nullptr))
  {
    LOG_ERROR("%s: could not load data file: %s", __func__, data_filename);
    return 1;
  }

  wsclient::map.setItemTypes(&wsclient::itemtypes);

  if (!wsclient::graphics::init(&wsclient::itemtypes, sprite_filename))
  {
    LOG_ERROR("%s: could not initialize graphics", __func__);
    return 1;
  }

  const auto usp = emscripten::val::global("URLSearchParams").new_(emscripten::val::global("location")["search"]);
  const auto uri = usp.call<bool>("has", emscripten::val("uri")) ?
                   usp.call<std::string>("get", emscripten::val("uri")) :
                   "ws://localhost:8172";
  LOG_INFO("%s: found uri: '%s'", __func__, uri.c_str());
  wsclient::network::start(uri, &wsclient::handle_packet);

  LOG_INFO("%s: starting main loop", __func__);
  emscripten_set_main_loop(main_loop, 0, true);
}
