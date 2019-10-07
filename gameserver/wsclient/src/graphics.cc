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
#include "itemtexture.h"

namespace
{

constexpr auto tile_size = 32;
constexpr auto scale = 2;
constexpr auto tile_size_scaled = tile_size * scale;

constexpr auto screen_width  = wsclient::consts::draw_tiles_x * tile_size_scaled;
constexpr auto screen_height = wsclient::consts::draw_tiles_y * tile_size_scaled;

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

SDL_Window* sdl_window = nullptr;
SDL_Renderer* sdl_renderer = nullptr;
io::data_loader::ItemTypes itemtypes;
io::SpriteLoader sprite_loader;
std::vector<wsclient::ItemTexture> item_textures;

/*
 * Item -> ItemType -> Sprites -> Texture info
 *
 * Item has an ItemType (ItemTypeId)
 * ItemType has sprite information:
 *  width:     >1 if the full sprite has more than 1 sprite in width
 *  height:    >1 if the full sprite has more than 1 sprite in height
 *  extra:     only used if width > 1 or height > 1, see drawItem()
 *  blend:     used for sprites with custom color, e.g. player sprites,
 *             1 template sprite, 1 color sprite, and so on
 *             valid values 1 and 2?
 *  xdiv:      different sprites for different (global) position in x
 *  ydiv:      different sprites for different (global) position in y
 *  num_anims: number of animations
 *
 * Total number of sprites: width * height * blend * xdiv * ydiv * num_anim
 *
 * Texture is a "full" sprite, e.g. full width and height
 *
 * Total number of textures: xdiv * ydiv * num_anim
 *
 * Select texture based on global position and animation tick
 * This is only valid for non blend sprites (all but player sprites?)
 */

SDL_Texture* createTexture(const std::vector<std::uint16_t>& sprite_ids,
                           std::uint8_t width,
                           std::uint8_t height,
                           std::uint8_t extra)
{
  // For now, ignore extra and always create the texture either
  // 32x32, 64x32, 32x64 or 64x64
  extra = 64U;

  const auto full_width = width == 1U ? 32U : extra;
  const auto full_height = height == 1U ? 32U : extra;

  // Validate number of sprites
  if (width * height != sprite_ids.size())
  {
    LOG_ERROR("%s: unexpected number of sprite ids: %u with width: %u height: %u",
              __func__,
              sprite_ids.size(),
              width,
              height);
    return nullptr;
  }

  // Combinations:
  //
  // width == 1 && height == 1 (32 x 32):
  //    A
  //
  // width == 2 && height == 1 (extra x 32):
  //   BA
  //
  // width == 1 && height == 2 (32 x extra): (see hack below)
  //    C
  //    A
  //
  // width == 2 && height == 2 (extra x extra):
  //   DC
  //   BA

  std::vector<std::uint8_t> texture_pixels(full_width * full_height * 4);
  for (auto i = 0U; i < sprite_ids.size(); i++)
  {
    const auto sprite_pixels = sprite_loader.getSpritePixels(sprite_ids[i]);

    // Hack to treat the two sprites as A and C when width == 1 and height == 2
    if (i == 1 && width == 1 && height == 2)
    {
      i = 2;
    }

    // Where to start writing the pixels on texture_pixels
    const auto start_x = (i == 0 || i == 2) && width == 2 ? 32U : 0U;
    const auto start_y = (i == 0 || i == 1) && height == 2 ? 32U : 0U;

    // Copy sprite pixels to texture pixels one row at a time
    auto it_source = sprite_pixels.begin();
    auto it_dest = texture_pixels.begin() + (start_y * full_width * 4) + (start_x * 4);
    for (auto y = 0; y < 32; y++)
    {
      std::copy(it_source,
                it_source + (32 * 4),
                it_dest);

      it_source += (32 * 4);
      it_dest += (full_width * 4);
    }
  }

  // Create surface and texture from pixels
  auto* surface = SDL_CreateRGBSurfaceFrom(texture_pixels.data(),
                                           full_width,
                                           full_height,
                                           32,
                                           full_width * 4,
                                           rmask,
                                           gmask,
                                           bmask,
                                           amask);
  if (!surface)
  {
    LOG_ERROR("%s: could not create surface: %s", __func__, SDL_GetError());
    return nullptr;
  }

  auto* texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);
  SDL_FreeSurface(surface);
  if (!texture)
  {
    LOG_ERROR("%s: could not create texture: %s", __func__, SDL_GetError());
    return nullptr;
  }

  return texture;
}

const std::vector<SDL_Texture*>& getTextures(common::ItemTypeId item_type_id)
{
  static const std::vector<SDL_Texture*> empty;

  auto it = std::find_if(item_textures.cbegin(),
                         item_textures.cend(),
                         [&item_type_id](const wsclient::ItemTexture& item_texture)
  {
    return item_texture.item_type_id == item_type_id;
  });

  // Create textures if not found
  if (it == item_textures.end())
  {
    wsclient::ItemTexture item_texture;
    item_texture.item_type_id = item_type_id;
    const auto& item_type = itemtypes[item_type_id];

    if (item_type.sprite_blend_frames != 1U)
    {
      LOG_ERROR("%s: item with blend not supported! (id: %u)", __func__, item_type_id);
      return empty;
    }

    const auto num_sprites_per_texture = item_type.sprite_width * item_type.sprite_height;
    const auto num_textures = item_type.sprite_xdiv * item_type.sprite_ydiv * item_type.sprite_num_anim;
    auto sprite_it = item_type.sprites.begin();
    for (auto i = 0; i < num_textures; i++)
    {
      const auto sprite_ids = std::vector<std::uint16_t>(sprite_it,
                                                         sprite_it + num_sprites_per_texture);
      auto* texture = createTexture(sprite_ids,
                                    item_type.sprite_width,
                                    item_type.sprite_height,
                                    item_type.sprite_extra);
      if (!texture)
      {
        LOG_ERROR("%s: could not create texture for item type id: %u", __func__, item_type_id);
        return empty;
      }
      item_texture.textures.push_back(texture);
      sprite_it += num_sprites_per_texture;
    }

    item_textures.push_back(item_texture);

    it = item_textures.end() - 1;
  }

  return it->textures;
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
