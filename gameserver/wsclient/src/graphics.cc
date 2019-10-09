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
#include <SDL.h>

#include "logger.h"
#include "data_loader.h"
#include "sprite_loader.h"
#include "texture.h"

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
std::vector<wsclient::Texture> item_textures;

const std::vector<SDL_Texture*>& getTextures(common::ItemTypeId item_type_id)
{
  auto it = std::find_if(item_textures.cbegin(),
                         item_textures.cend(),
                         [&item_type_id](const wsclient::Texture& item_texture)
  {
    return item_texture.getItemTypeId() == item_type_id;
  });

  // Create textures if not found
  if (it == item_textures.end())
  {
    const auto& item_type = itemtypes[item_type_id];
    item_textures.push_back(wsclient::Texture::create(sdl_renderer,
                                                      sprite_loader,
                                                      item_type));
    it = item_textures.end() - 1;
  }

  return it->getTextures();
}

void drawItem(int x, int y, const common::ItemType& item_type, std::uint16_t offset, int anim_tick)
{
  static std::vector<common::ItemTypeId> no_texture;

  if (std::find(no_texture.begin(), no_texture.end(), item_type.id) != no_texture.end())
  {
    return;
  }

  const auto& textures = getTextures(item_type.id);
  if (textures.empty())
  {
    LOG_ERROR("%s: missing texture for item type id: %u", __func__, item_type.id);
    no_texture.push_back(item_type.id);
    return;
  }

  // Need to use global positon, not local
  const auto xdiv = item_type.sprite_xdiv == 0 ? 0 : x % item_type.sprite_xdiv;
  const auto ydiv = item_type.sprite_ydiv == 0 ? 0 : y % item_type.sprite_ydiv;
  const auto texture_index = xdiv + (ydiv * item_type.sprite_xdiv) + (anim_tick % item_type.sprite_num_anim);
  if (texture_index < 0 || texture_index >= static_cast<int>(textures.size()))
  {
    LOG_ERROR("%s: texture_index: %d is invalid (textures.size(): %d)",
              __func__,
              texture_index,
              static_cast<int>(textures.size()));
    return;
  }

  // Convert from tile position to pixel position
  // TODO: there is probably a max offset...
  const SDL_Rect dest
  {
    (x * tile_size - offset - ((item_type.sprite_width - 1) * 32)) * scale,
    (y * tile_size - offset - ((item_type.sprite_height - 1) * 32)) * scale,
    item_type.sprite_width * tile_size_scaled,
    item_type.sprite_height * tile_size_scaled
  };
  SDL_RenderCopy(sdl_renderer, textures[texture_index], nullptr, &dest);
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
  const auto anim_tick = SDL_GetTicks() / 540;

  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);

  if (!map.ready())
  {
    SDL_RenderPresent(sdl_renderer);
    return;
  }

  for (auto y = 0; y < consts::draw_tiles_y; y++)
  {
    for (auto x = 0; x < consts::draw_tiles_x; x++)
    {
      const auto& tile = map.getTile(common::Position(x - 7 + position.getX(),
                                                      y - 5 + position.getY(),
                                                      position.getZ()));

      // Draw ground
      const auto& ground_type = itemtypes[tile.things.front().item.item_type_id];
      drawItem(x, y, ground_type, 0, anim_tick);

      // Draw things in reverse order, except ground
      auto offset = ground_type.offset;
      for (auto it = tile.things.rbegin(); it != tile.things.rend() - 1; ++it)
      {
        const auto& thing = *it;
        if (thing.is_item)
        {
          // TODO: probably need things like count later
          const auto& item_type = itemtypes[thing.item.item_type_id];
          drawItem(x, y, item_type, offset, anim_tick);

          offset += item_type.offset;
        }
        // Creature: TODO
      }
    }
  }

  SDL_RenderPresent(sdl_renderer);
}

}  // namespace wsclient::graphics
