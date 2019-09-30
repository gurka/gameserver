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
#include "data_loader.h"
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
io::data_loader::ItemTypes itemtypes;
io::SpriteLoader sprite_loader;

struct Texture
{
  std::uint16_t sprite_id;
  SDL_Texture* texture;
};

std::vector<Texture> textures;

SDL_Texture* getTexture(std::uint16_t sprite_id)
{
  const auto it = std::find_if(textures.cbegin(),
                               textures.cend(),
                               [&sprite_id](const Texture& texture)
  {
    return texture.sprite_id == sprite_id;
  });

  if (it != textures.cend())
  {
    return it->texture;
  }

  // Load new texture
  // Cannot be const due to SDL_CreateRGBSurfaceFrom
  auto pixels = sprite_loader.getSpritePixels(sprite_id);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  constexpr auto rmask = 0xFF000000U;
  constexpr auto gmask = 0x00FF0000U;
  constexpr auto bmask = 0x0000FF00U;
  constexpr auto amask = 0x000000FFU;
#else
  constexpr auto rmask = 0x000000FFU;
  constexpr auto gmask = 0x0000FF00U;
  constexpr auto bmask = 0x00FF0000U;
  constexpr auto amask = 0xFF000000U;
#endif
  auto* surface = SDL_CreateRGBSurfaceFrom(pixels.data(), 32, 32, 32, 32 * 4, rmask, gmask, bmask, amask);
  if (!surface)
  {
    LOG_ERROR("%s: could not create surface: %s", __func__, SDL_GetError());
    return nullptr;
  }

  // Create texture from surface
  auto* texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);
  if (!texture)
  {
    LOG_ERROR("%s: could not create texture: %s", __func__, SDL_GetError());
    return nullptr;
  }

  SDL_FreeSurface(surface);

  // Add to cache
  textures.push_back(Texture{sprite_id, texture});

  return texture;
}

void drawTexture(int x, int y, SDL_Texture* texture)
{
  const SDL_Rect dest { x * scale, y * scale, tile_size_scaled, tile_size_scaled };
  SDL_RenderCopy(sdl_renderer, texture, nullptr, &dest);
}

void drawItem(int x, int y, const common::ItemType& item_type, std::uint16_t offset)
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

  auto* texture = getTexture(item_type.sprites[sprite_index++]);
  if (!texture)
  {
    LOG_ERROR("%s: missing texture for sprite id: %d", __func__, sprite_index - 1);
    return;
  }

  // Convert from tile position to pixel position
  // TODO: there is probably a max offset...
  x = x * tile_size - offset;
  y = y * tile_size - offset;

  drawTexture(x, y, texture);

  // TODO: Take care of this when loading texture, e.g. combine the extra sprites
  //       into one single texture
  if (item_type.sprite_extra > 0)
  {
    // sprite_extra = n means that the sprite is not 32 x 32 but rather:
    // if sprite_width = 2 and sprite_height = 1: n x 32
    // if sprite_width = 1 and sprite_height = 2: 32 x n
    // if sprite_width = 2 and sprite_height = 2: n x n
    //
    if (item_type.sprite_width == 2)
    {
      texture = getTexture(item_type.sprites[sprite_index++]);
      drawTexture(x - tile_size, y, texture);
    }

    if (item_type.sprite_height == 2)
    {
      texture = getTexture(item_type.sprites[sprite_index++]);
      drawTexture(x, y - tile_size, texture);
    }

    if (item_type.sprite_width == 2 && item_type.sprite_height == 2)
    {
      texture = getTexture(item_type.sprites[sprite_index++]);
      drawTexture(x - tile_size, y - tile_size, texture);
    }
  }
}

}  // namespace

namespace wsclient::graphics
{

bool init(const std::string& data_filename, const std::string& sprite_filename)
{
  if (!io::data_loader::load(data_filename, &itemtypes, nullptr, nullptr))
  {
    LOG_ERROR("Could not load \"%s\"", data_filename.c_str());
    return false;
  }

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

  if (!sprite_loader.load(sprite_filename))
  {
    LOG_ERROR("%s: could not sprites", __func__);
    return false;
  }

  return true;
}

void draw(const wsworld::Map& map,
          const common::Position& position,
          common::CreatureId player_id)
{
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);

  for (auto y = 0; y < consts::draw_tiles_y; y++)
  {
    for (auto x = 0; x < consts::draw_tiles_x; x++)
    {
      const auto& tile = map.getTile(common::Position(x - 7 + position.getX(),
                                                      y - 5 + position.getY(),
                                                      position.getZ()));

      // Draw ground
      const auto& ground_type = itemtypes[tile.things.front().item.item_type_id];
      drawItem(x, y, ground_type, 0);

      // Draw things in reverse order, except ground
      auto offset = ground_type.offset;
      for (auto it = tile.things.rbegin(); it != tile.things.rend() - 1; ++it)
      {
        const auto& thing = *it;
        if (thing.is_item)
        {
          // TODO: probably need things like count later
          const auto& item_type = itemtypes[thing.item.item_type_id];
          drawItem(x, y, item_type, offset);

          offset += item_type.offset;
        }
        // Creature: TODO
      }
    }
  }

  SDL_RenderPresent(sdl_renderer);
}

}  // namespace wsclient::graphics
