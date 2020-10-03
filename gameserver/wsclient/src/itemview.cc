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
#include <sstream>
#include <string>
#include <vector>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <SDL.h>
#else
#include <asio.hpp>
#include <SDL2/SDL.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

#include "logger.h"
#include "data_loader.h"
#include "sprite_loader.h"
#include "texture.h"

constexpr auto DATA_FILENAME = "files/data.dat";
constexpr auto SPRITE_FILENAME = "files/sprite.dat";

constexpr auto SCREEN_WIDTH = 320;
constexpr auto SCREEN_HEIGHT = 320;
constexpr auto TILE_SIZE = 32;

constexpr auto SCALE = 2;
constexpr auto SCREEN_WIDTH_SCALED = SCREEN_WIDTH * SCALE;
constexpr auto SCREEN_HEIGHT_SCALED = SCREEN_HEIGHT * SCALE;
constexpr auto TILE_SIZE_SCALED = TILE_SIZE * SCALE;

SDL_Renderer* sdl_renderer = nullptr;

wsclient::SpriteLoader sprite_loader;
utils::data_loader::ItemTypes item_types;
common::ItemTypeId item_type_id_first;
common::ItemTypeId item_type_id_last;
common::ItemType item_type;
wsclient::Texture texture;

void logItem(const common::ItemType& item_type)
{
  std::ostringstream ss;
  item_type.dump(&ss, false);
  LOG_INFO(ss.str().c_str());
}

void setItemType(common::ItemTypeId item_type_id)
{
  item_type = item_types[item_type_id];
  texture = wsclient::Texture::create(sdl_renderer, sprite_loader, item_type);
  logItem(item_type);
}

void render()
{
  const auto anim_tick = SDL_GetTicks() / 540;

  SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);
  SDL_RenderClear(sdl_renderer);

  const auto direction = item_type.type == common::ItemType::Type::CREATURE &&
                         item_type.sprite_xdiv == 4;

  if (direction)
  {
    // Creature with 4 sets of textures for each direction

    // First row is standing still (animation index 0)
    for (auto dir = 0; dir < 4; dir++)
    {
      auto* sdl_texture = texture.getCreatureStillTexture(common::Direction(dir));
      if (!sdl_texture)
      {
        continue;
      }
      const SDL_Rect dest
      {
        dir * item_type.sprite_width * TILE_SIZE_SCALED,
        0 * item_type.sprite_height * TILE_SIZE_SCALED,
        item_type.sprite_width * TILE_SIZE_SCALED,
        item_type.sprite_height * TILE_SIZE_SCALED
      };
      SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, &dest);
    }

    if (item_type.sprite_num_anim > 1)
    {
      // Second row is walking (animation index 1..n)
      for (auto dir = 0; dir < 4; dir++)
      {
        // Use anim_tick as walk_tick
        auto* sdl_texture = texture.getCreatureWalkTexture(common::Direction(dir), anim_tick);
        if (!sdl_texture)
        {
          continue;
        }
        const SDL_Rect dest
        {
          dir * item_type.sprite_width * TILE_SIZE_SCALED,
          1 * item_type.sprite_height * TILE_SIZE_SCALED,
          item_type.sprite_width * TILE_SIZE_SCALED,
          item_type.sprite_height * TILE_SIZE_SCALED
        };
        SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, &dest);
      }
    }
  }
  else
  {
    // Normal item: render all blends, xdivs and ydivs
    for (auto y = 0; y < item_type.sprite_ydiv; y++)
    {
      for (auto x = 0; x < item_type.sprite_xdiv; x++)
      {
        auto* sdl_texture = texture.getItemTexture(common::Position(x, y, 0), anim_tick);
        if (!sdl_texture)
        {
          continue;
        }

        const SDL_Rect dest
        {
          x * item_type.sprite_width * TILE_SIZE_SCALED,
          y * item_type.sprite_height * TILE_SIZE_SCALED,
          item_type.sprite_width * TILE_SIZE_SCALED,
          item_type.sprite_height * TILE_SIZE_SCALED
        };
        SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, &dest);
      }
    }
  }

  SDL_RenderPresent(sdl_renderer);
}

#ifndef EMSCRIPTEN
static asio::io_context io_context;
static bool stop = false;
static std::function<void(void)> main_loop_func;
static std::unique_ptr<asio::deadline_timer> timer;

static const int TARGET_FPS = 60;

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
  timer->expires_from_now(boost::posix_time::millisec(1000 / TARGET_FPS));
  timer->async_wait(&timerCallback);
}

void emscripten_set_main_loop(std::function<void(void)> func, int fps, int loop)  // NOLINT
{
  (void)fps;
  (void)loop;

  main_loop_func = std::move(func);
  timer = std::make_unique<asio::deadline_timer>(io_context);
  timer->expires_from_now(boost::posix_time::millisec(1000 / TARGET_FPS));
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

void handleEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event) == 1)
  {
    if (event.type == SDL_KEYUP)
    {
      if (event.key.keysym.sym == SDLK_RIGHT && item_type.id != item_type_id_last)
      {
        setItemType(item_type.id + 1);
      }
      else if (event.key.keysym.sym == SDLK_LEFT && item_type.id != item_type_id_first)
      {
        setItemType(item_type.id - 1);
      }
      else if (event.key.keysym.sym == SDLK_ESCAPE)
      {
        stop = true;
      }
    }
    else if (event.type == SDL_QUIT)
    {
      stop = true;
    }
  }
}

extern "C" void mainLoop()
{
  render();
  handleEvents();
}

int main()
{
  // Load data
  if (!utils::data_loader::load(DATA_FILENAME, &item_types, &item_type_id_first, &item_type_id_last))
  {
    LOG_ERROR("Could not load data");
    return 1;
  }

  // Load sprites
  if (!sprite_loader.load(SPRITE_FILENAME))
  {
    LOG_ERROR("Could not load sprites");
    return 1;
  }

  // Load SDL
  SDL_Init(SDL_INIT_VIDEO);
  auto* sdl_window = SDL_CreateWindow("itemview",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SCREEN_WIDTH_SCALED,
                                      SCREEN_HEIGHT_SCALED,
                                      0);
  if (!sdl_window)
  {
    LOG_ERROR("%s: could not create window: %s", __func__, SDL_GetError());
    return 1;
  }

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  if (!sdl_renderer)
  {
    LOG_ERROR("%s: could not create renderer: %s", __func__, SDL_GetError());
    return 1;
  }

  // Load initial item type
  // First creature (monster): 2284
  // First creature (outfit): 2410
  setItemType(3134);

  LOG_INFO("itemview started");

  emscripten_set_main_loop(mainLoop, 0, 1);
}

