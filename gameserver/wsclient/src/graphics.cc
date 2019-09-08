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

namespace Graphics
{
  const auto tile_size = 32;
  const auto scale = 2;
  const auto width  = types::draw_tiles_x * tile_size * scale;
  const auto height = types::draw_tiles_y * tile_size * scale;

  SDL_Surface* screen = nullptr;

  void fillRect(int x, int y, int width, int height, int red, int green, int blue)
  {
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

  void draw(const types::Map& map, const Position& position, std::uint32_t playerId)
  {
    if (SDL_MUSTLOCK(screen))
    {
      SDL_LockSurface(screen);
    }

    for (auto tile_y = 0; tile_y < types::draw_tiles_y; tile_y++)
    {
      for (auto tile_x = 0; tile_x < types::draw_tiles_x; tile_x++)
      {
        // Adjust for draw/known map size
        const auto& tile = map[1 + tile_y][1 + tile_x];

        //  Draw creature over tile
        if (!tile.creatures.empty())
        {
          if (tile.creatures.front().creature.id == playerId)
          {
            // Fill sprite with green
            fillRect(tile_x * tile_size,
                     tile_y * tile_size,
                     tile_size,
                     tile_size,
                     0,
                     255,
                     0);
          }
          else
          {
            // Fill sprite with red
            fillRect(tile_x * tile_size,
                     tile_y * tile_size,
                     tile_size,
                     tile_size,
                     255,
                     0,
                     0);
          }
        }
        else if (!tile.items.empty())
        {
          const auto& ground = tile.items.front();
          if (ground.item.itemTypeId == 694)
          {
            // Ground, fill with brown
            fillRect(tile_x * tile_size,
                     tile_y * tile_size,
                     tile_size,
                     tile_size,
                     218,
                     165,
                     32);

          }
          else if (ground.item.itemTypeId == 475)
          {
            // Water, fill with blue
            fillRect(tile_x * tile_size,
                     tile_y * tile_size,
                     tile_size,
                     tile_size,
                     0,
                     0,
                     255);
          }
          else
          {
            // Unknown itemTypeId, fill with black
            LOG_INFO("%s: unknown itemTypeId: %d", __func__, ground.item.itemTypeId);
            fillRect(tile_x * tile_size,
                     tile_y * tile_size,
                     tile_size,
                     tile_size,
                     0,
                     0,
                     0);
          }
        }
        else
        {
          // Empty tile, fill with black
          fillRect(tile_x * tile_size,
                   tile_y * tile_size,
                   tile_size,
                   tile_size,
                   0,
                   0,
                   0);
        }
      }
    }

    if (SDL_MUSTLOCK(screen))
    {
      SDL_UnlockSurface(screen);
    }
  }
}
