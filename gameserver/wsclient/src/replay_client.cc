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
#include <functional>
#include <memory>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/val.h>
#include <SDL.h>
#else
#include <asio/error_code.hpp>
#include <asio/deadline_timer.hpp>
#include <asio/io_context.hpp>
#include <SDL2/SDL.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

#include "network/incoming_packet.h"
#include "protocol/protocol_common.h"
#include "utils/data_loader.h"
#include "utils/logger.h"
#include "common/position.h"

#include "main_ui.h"
#include "game/game.h"
#include "game/game_ui.h"
#include "game/sprite_loader.h"
#include "protocol.h"
#include "replay_reader.h"

namespace replay_client
{

utils::data_loader::ItemTypes item_types;
std::unique_ptr<game::SpriteLoader> sprite_loader;
std::unique_ptr<game::Game> game;
std::unique_ptr<game::GameUI> game_ui;

std::unique_ptr<Protocol> protocol;

std::unique_ptr<Replay> replay;

#ifndef EMSCRIPTEN
asio::io_context io_context;
bool stop = false;
std::function<void(void)> main_loop_func;
std::unique_ptr<asio::deadline_timer> timer;
const int TARGET_FPS = 60;
#endif

}  // namespace replay_client

#ifndef EMSCRIPTEN
void timerCallback(const asio::error_code& ec)
{
  if (ec)
  {
    LOG_ERROR("%s: ec: %s", __func__, ec.message().c_str());
    replay_client::stop = true;
    return;
  }

  if (replay_client::stop)
  {
    LOG_INFO("%s: stop=true", __func__);
    return;
  }

  replay_client::main_loop_func();
  replay_client::timer->expires_from_now(boost::posix_time::millisec(1000 / replay_client::TARGET_FPS));
  replay_client::timer->async_wait(&timerCallback);
}

void emscripten_set_main_loop(std::function<void(void)> func, int fps, int loop)  // NOLINT
{
  (void)fps;
  (void)loop;

  replay_client::main_loop_func = std::move(func);
  replay_client::timer = std::make_unique<asio::deadline_timer>(replay_client::io_context);
  replay_client::timer->expires_from_now(boost::posix_time::millisec(1000 / replay_client::TARGET_FPS));
  replay_client::timer->async_wait(&timerCallback);
  replay_client::io_context.run();
}

void emscripten_cancel_main_loop()  // NOLINT
{
  if (!replay_client::stop)
  {
    replay_client::stop = true;
    replay_client::timer->cancel();
  }
}
#endif

extern "C" void main_loop()  // NOLINT
{
  // Check if it's time to "send" next packet in recording
  while (replay_client::replay->timeForNextPacket())
  {
    LOG_INFO("%s: sending a packet!", __func__);
    const auto outgoing_packet = replay_client::replay->getNextPacket();
    network::IncomingPacket incoming_packet(outgoing_packet.getBuffer() + 2, outgoing_packet.getLength() - 2);
    replay_client::protocol->handlePacket(&incoming_packet);
  }

  // Read input
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0)
  {
    if (event.type == SDL_KEYDOWN)
    {
      switch (event.key.keysym.scancode)
      {
        case SDL_SCANCODE_ESCAPE:
          LOG_INFO("%s: stopping client", __func__);
          emscripten_cancel_main_loop();
          return;

          /*
        case SDL_SCANCODE_RETURN:
          if (map.ready())
          {
            // Print player position, number of known floors ... more?
            LOG_ERROR("%s: position: %s number of known floors: %d",
                      __func__,
                      map.getPlayerPosition().toString().c_str(),
                      map.getNumFloors());
          }
          break;
          */

        default:
          break;
      }
    }
    /*
    else if (event.type == SDL_MOUSEBUTTONDOWN)
    {
      // note: z is not set by screenToMapPosition
      const auto map_position = graphics::screenToMapPosition(event.button.x, event.button.y);

      // Convert to global position
      const auto& player_position = map.getPlayerPosition();
      const auto global_position = common::Position(player_position.getX() - 8 + map_position.getX(),
                                                    player_position.getY() - 6 + map_position.getY(),
                                                    player_position.getZ());
      const auto* tile = map.getTile(global_position);
      if (tile)
      {
        LOG_INFO("Tile at %s", global_position.toString().c_str());
        int stackpos = 0;
        for (const auto& thing : tile->things)
        {
          if (std::holds_alternative<wsworld::Item>(thing))
          {
            const auto& item = std::get<wsworld::Item>(thing);
            std::ostringstream oss;
            oss << "  stackpos=" << stackpos << " ";
            item.type->dump(&oss, false);
            oss << " [extra=" << static_cast<int>(item.extra) << "]\n";
            LOG_INFO(oss.str().c_str());
          }
          else if (std::holds_alternative<common::CreatureId>(thing))
          {
            const auto creature_id = std::get<common::CreatureId>(thing);
            const auto* creature = static_cast<const wsworld::Map*>(&map)->getCreature(creature_id);
            if (creature)
            {
              LOG_INFO("  stackpos=%d Creature [id=%d, name=%s]", stackpos, creature_id, creature->name.c_str());
            }
            else
            {
              LOG_ERROR("  stackpos=%d: creature with id=%d is nullptr", stackpos, creature_id);
            }
          }
          else
          {
            LOG_ERROR("  stackpos=%d: invalid Thing on Tile", stackpos);
          }

          ++stackpos;
        }
      }
      else
      {
        LOG_ERROR("%s: clicked on invalid tile", __func__);
      }
    }
     */
    else if (event.type == SDL_WINDOWEVENT)
    {
      if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      {
        //graphics::setWindowSize(event.window.data1, event.window.data2);
      }
    }
    else if (event.type == SDL_QUIT)
    {
      LOG_INFO("%s: stopping client", __func__);
      emscripten_cancel_main_loop();
      return;
    }
  }

  // Render
  main_ui::render();
}

int main()
{
  constexpr auto data_filename = "files/data.dat";
  constexpr auto sprite_filename = "files/sprite.dat";

  if (!utils::data_loader::load(data_filename, &replay_client::item_types, nullptr, nullptr))
  {
    LOG_ERROR("%s: could not load data file: %s", __func__, data_filename);
    return 1;
  }

  replay_client::sprite_loader = std::make_unique<game::SpriteLoader>();
  if (!replay_client::sprite_loader->load(sprite_filename))
  {
    LOG_ERROR("%s: could not load sprite file: %s", __func__, sprite_filename);
    return 1;
  }

  protocol::setItemTypes(&replay_client::item_types);

  // Create Game model
  replay_client::game = std::make_unique<game::Game>();
  replay_client::game->setItemTypes(&replay_client::item_types);

  // Create Protocol
  replay_client::protocol = std::make_unique<Protocol>(replay_client::game.get());

  // Create UI
  main_ui::init();
  replay_client::game_ui = std::make_unique<game::GameUI>(replay_client::game.get(),
                                                          main_ui::get_renderer(),
                                                          replay_client::sprite_loader.get(),
                                                          &replay_client::item_types);
  main_ui::setGameUI(replay_client::game_ui.get());

  LOG_INFO("%s: loading replay", __func__);
  replay_client::replay = std::make_unique<Replay>();
  if (!replay_client::replay->load("replay.trp"))
  {
    LOG_ERROR("%s: could not load replay.trp: ", __func__, replay_client::replay->getErrorStr().c_str());
    return 1;
  }
  LOG_INFO("%s: replay info: version=%d length=%d", __func__, replay_client::replay->getVersion(), replay_client::replay->getLength());

  LOG_INFO("%s: starting main loop", __func__);
  emscripten_set_main_loop(main_loop, 0, 1);
}
