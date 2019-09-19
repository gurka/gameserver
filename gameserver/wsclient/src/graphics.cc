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
#include "item_types.h"
#include "sprite_loader.h"

namespace
{

constexpr auto tile_size = 32;
constexpr auto scale = 2;
constexpr auto tile_size_scaled = tile_size * scale;

constexpr auto screen_width  = wsclient::consts::draw_tiles_x * tile_size_scaled;
constexpr auto screen_height = wsclient::consts::draw_tiles_y * tile_size_scaled;

SDL_Window* sdl_window = nullptr;
SDL_Renderer* sdl_renderer = nullptr;
wsclient::wsworld::ItemTypes itemtypes;
wsclient::sprite::Reader sprite_reader;

void drawTexture(int x, int y, SDL_Texture* texture)
{
  const SDL_Rect dest { x, y, tile_size_scaled, tile_size_scaled };
  SDL_RenderCopy(sdl_renderer, texture, nullptr, &dest);
}

void drawItem(int x, int y, const wsclient::wsworld::ItemType& item_type)
{
  // Need to use global positon, not local
  const auto xdiv = item_type.sprite_xdiv == 0 ? 0 : x % item_type.sprite_xdiv;
  const auto ydiv = item_type.sprite_ydiv == 0 ? 0 : y % item_type.sprite_ydiv;
  auto sprite_index = xdiv + (ydiv * item_type.sprite_xdiv);
  if (sprite_index < 0 || sprite_index >= static_cast<int>(item_type.sprites.size()))
  {
    LOG_ERROR("%s: sprite_index: %d is invalid (sprites.size(): %d)",
              __func__,
              sprite_index,
              static_cast<int>(item_type.sprites.size()));
    return;
  }

  auto* texture = sprite_reader.get_sprite(item_type.sprites[sprite_index++], sdl_renderer);
  if (!texture)
  {
    return;
  }

  drawTexture(x * tile_size_scaled, y * tile_size_scaled, texture);

  // TODO: Take care of this when loading texture, e.g. combine the extra sprites
  //       into one single texture
  if (item_type.sprite_extra > 0)
  {
    // sprite_extra = n means that the sprite is not 32 x 32 but rather:
    // if sprite_width = 2 and sprite_height = 1: n x 32
    // if sprite_width = 1 and sprite_height = 2: 32 x n
    // if sprite_width = 2 and sprite_height = 2: n x n
    //
    x *= tile_size;
    y *= tile_size;
    if (item_type.sprite_width == 2)
    {
      texture = sprite_reader.get_sprite(item_type.sprites[sprite_index++], sdl_renderer);
      drawTexture((x - tile_size) * scale,
                  y * scale,
                  texture);
    }

    if (item_type.sprite_height == 2)
    {
      texture = sprite_reader.get_sprite(item_type.sprites[sprite_index++], sdl_renderer);
      drawTexture(x * scale,
                  (y - tile_size) * scale,
                  texture);
    }

    if (item_type.sprite_width == 2 && item_type.sprite_height == 2)
    {
      texture = sprite_reader.get_sprite(item_type.sprites[sprite_index++], sdl_renderer);
      drawTexture((x - tile_size) * scale,
                  (y - tile_size) * scale,
                  texture);
    }
  }
}

}  // namespace

namespace wsclient::graphics
{

bool init(const std::string& data_filename, const std::string& sprite_filename)
{
  const auto tmp = item_types::load(data_filename);
  if (!tmp)
  {
    LOG_ERROR("Could not load \"%s\"", data_filename.c_str());
    return false;
  }
  itemtypes = tmp.value();

  SDL_Init(SDL_INIT_VIDEO);

  sdl_window = SDL_CreateWindow("wsclient", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, 0);
  if (!sdl_window)
  {
    LOG_ERROR("%s: could not create window: %s", __func__, SDL_GetError());
    return false;
  }

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  if (!sdl_renderer)
  {
    LOG_ERROR("%s: could not create renderer: %s", __func__, SDL_GetError());
    return false;
  }

  if (!sprite_reader.load(sprite_filename))
  {
    LOG_ERROR("%s: could not sprites", __func__);
    return false;
  }

  return true;
}

void draw(const wsworld::Map& map,
          const wsworld::Position& position,
          wsworld::CreatureId player_id)
{
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);

  for (auto y = 0; y < consts::draw_tiles_y; y++)
  {
    for (auto x = 0; x < consts::draw_tiles_x; x++)
    {
      const auto& tile = map.getTile(world::Position(x - 7 + position.getX(),
                                                     y - 5 + position.getY(),
                                                     position.getZ()));

      // Draw ground
      const auto& ground = tile.things.front();
      drawItem(x, y, itemtypes[ground.item.item_type_id]);

      // Draw things in reverse order, except ground
      for (auto it = tile.things.rbegin(); it != tile.things.rend() - 1; ++it)
      {
        const auto& thing = *it;
        if (thing.is_item)
        {
          // TODO: probably need things like count later
          const auto& item_type = itemtypes[thing.item.item_type_id];
          drawItem(x, y, item_type);

          // TODO: add offset to item_type and draw things on top of it
          //       with an offset, e.g. boxes
        }
        // Creature: TODO
      }
    }
  }

  SDL_RenderPresent(sdl_renderer);
}

}  // namespace wsclient::graphics
