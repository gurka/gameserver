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
#include "graphics.h"

#include <cstdint>

#include <emscripten.h>
#include <SDL/SDL.h>

#include "logger.h"

namespace wsclient::graphics
{

constexpr auto tile_size = 32;
constexpr auto scale = 2;
constexpr auto width  = consts::draw_tiles_x * tile_size * scale;
constexpr auto height = consts::draw_tiles_y * tile_size * scale;

SDL_Surface* screen = nullptr;

void fillRect(int x, int y, int width, int height, int red, int green, int blue)
{
  // Probably UB...
  auto* pixel = reinterpret_cast<std::uint32_t*>(screen->pixels) + (y * scale * screen->w) + (x * scale);
  for (auto y = 0; y < height * scale; y++)
  {
    for (auto x = 0; x < width * scale; x++)
    {
      // Black border
      if (y == 0 || x == -0 || y == (height * scale) - 1 || x == (height * scale - 1))
      {
        *pixel = SDL_MapRGBA(screen->format, 0, 0, 0, 0);
      }
      else
      {
        *pixel = SDL_MapRGBA(screen->format, red, green, blue, 0);
      }
      pixel += 1;
    }
    pixel += screen->w - (width * scale);
  }
}

void init()
{
  SDL_Init(SDL_INIT_VIDEO);
  // Client displays 15x11 tiles
  screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
}

void draw(const world::Map& map, const world::Position& position, std::uint32_t player_id)
{
  if (SDL_MUSTLOCK(screen))
  {
    SDL_LockSurface(screen);
  }

  for (auto y = 0; y < consts::draw_tiles_y; y++)
  {
    for (auto x = 0; x < consts::draw_tiles_x; x++)
    {
      const auto& tile = map.getTile(world::Position(x - 7 + position.getX(),
                                                     y - 5 + position.getY(),
                                                     position.getZ()));
      for (const auto& thing : tile.things)
      {
        if (thing.is_item)
        {
          if (thing.item.item_type_id == 694)
          {
            // Ground, fill with brown
            fillRect(x * tile_size,
                     y * tile_size,
                     tile_size,
                     tile_size,
                     218,
                     165,
                     32);

          }
          else if (thing.item.item_type_id == 475)
          {
            // Water, fill with blue
            fillRect(x * tile_size,
                     y * tile_size,
                     tile_size,
                     tile_size,
                     0,
                     0,
                     255);
          }
          else
          {
            // Unknown item_type_id, fill with black
            LOG_INFO("%s: unknown item_type_id: %d", __func__, thing.item.item_type_id);
            fillRect(x * tile_size,
                     y * tile_size,
                     tile_size,
                     tile_size,
                     0,
                     0,
                     0);
          }
        }
        else
        {
          // Creature
          if (thing.creature_id == player_id)
          {
            // Fill sprite with green
            fillRect(x * tile_size,
                     y * tile_size,
                     tile_size,
                     tile_size,
                     0,
                     255,
                     0);
          }
          else
          {
            // Fill sprite with red
            fillRect(x * tile_size,
                     y * tile_size,
                     tile_size,
                     tile_size,
                     255,
                     0,
                     0);
          }
        }
      }
    }
  }

  if (SDL_MUSTLOCK(screen))
  {
    SDL_UnlockSurface(screen);
  }
}

}  // namespace wsclient::graphics
