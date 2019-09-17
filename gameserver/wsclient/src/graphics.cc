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

namespace
{

constexpr auto tile_size = 32;
constexpr auto scale = 2;
constexpr auto tile_size_scaled = tile_size * scale;

constexpr auto screen_width  = wsclient::consts::draw_tiles_x * tile_size_scaled;
constexpr auto screen_height = wsclient::consts::draw_tiles_y * tile_size_scaled;

SDL_Surface* screen = nullptr;
const std::array<wsclient::wsworld::ItemType, 4096>* item_types = nullptr;

void drawSprite(int x, int y, int red, int green, int blue)
{
  x *= tile_size;
  y *= tile_size;

  // Probably UB...
  auto* pixel = reinterpret_cast<std::uint32_t*>(screen->pixels) + (y * scale * screen->w) + (x * scale);
  for (auto y = 0; y < tile_size_scaled; y++)
  {
    for (auto x = 0; x < tile_size_scaled; x++)
    {
      // Black border
      if (y == 0 || x == -0 || y == tile_size_scaled - 1 || x == tile_size_scaled - 1)
      {
        *pixel = SDL_MapRGBA(screen->format, 0, 0, 0, 0);
      }
      else
      {
        *pixel = SDL_MapRGBA(screen->format, red, green, blue, 0);
      }
      pixel += 1;
    }
    pixel += screen->w - (tile_size * scale);
  }
}

}

namespace wsclient::graphics
{

void init(const wsworld::ItemTypes* item_types_in)
{
  SDL_Init(SDL_INIT_VIDEO);
  // Client displays 15x11 tiles
  screen = SDL_SetVideoMode(screen_width, screen_height, 32, SDL_SWSURFACE);
  item_types = item_types_in;
}

void draw(const wsworld::Map& map,
          const wsworld::Position& position,
          wsworld::CreatureId player_id)
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
          const auto& item_type = (*item_types)[thing.item.item_type_id];
          if (item_type.ground)
          {
            // Ground => brown
            drawSprite(x, y, 218, 165, 32);

          }
          else if (item_type.is_blocking)
          {
            // Blocking => gray
            drawSprite(x, y, 33, 33, 33);
          }
          else
          {
            // Other => black
            drawSprite(x, y, 0, 0, 0);
          }
        }
        else
        {
          // Creature
          if (thing.creature_id == player_id)
          {
            // Fill sprite with green
            drawSprite(x, y, 0, 255, 0);
          }
          else
          {
            // Fill sprite with red
            drawSprite(x, y, 255, 0, 0);
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
