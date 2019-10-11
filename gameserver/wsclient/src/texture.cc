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
#include "texture.h"

#include <SDL.h>

#include "logger.h"

namespace
{

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

io::SpriteLoader::SpritePixels blendSprites(const io::SpriteLoader::SpritePixels& bottom,
                                            const io::SpriteLoader::SpritePixels& top)
{
  io::SpriteLoader::SpritePixels result = bottom;
  for (auto i = 0U; i < result.size(); i += 4)
  {
    // Add pixel from top if it is not alpha
    if (top[i + 3] != 0x00U)
    {
      result[i + 0] = top[i + 0];
      result[i + 1] = top[i + 1];
      result[i + 2] = top[i + 2];
      result[i + 3] = 0xFFU;
    }
  }
  return result;
}

// TODO(simon): send in SpriteData directly?
SDL_Texture* createSDLTexture(SDL_Renderer* renderer,
                              const io::SpriteLoader& sprite_loader,
                              const std::vector<std::uint16_t>& sprite_ids,
                              std::uint8_t width,
                              std::uint8_t height,
                              std::uint8_t extra,
                              bool blend,
                              bool directions)
{
  // For now, ignore extra and always create the texture either
  // 32x32, 64x32, 32x64 or 64x64
  extra = 64U;

  const auto full_width = width == 1U ? 32U : extra;
  const auto full_height = height == 1U ? 32U : extra;

  // Validate number of sprites
  if (width * height * (blend ? 2 : 1) != sprite_ids.size())
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
  for (auto i = 0U; i < sprite_ids.size(); i += (blend ? 2 : 1))
  {
    const auto sprite_pixels = blend ? blendSprites(sprite_loader.getSpritePixels(sprite_ids[i + 0]),
                                                    sprite_loader.getSpritePixels(sprite_ids[i + 1]))
                                     : sprite_loader.getSpritePixels(sprite_ids[i]);

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

  auto* texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);
  if (!texture)
  {
    LOG_ERROR("%s: could not create texture: %s", __func__, SDL_GetError());
    return nullptr;
  }

  return texture;
}

}  // namespace

namespace wsclient
{

/*
 * Item -> ItemType -> Sprites -> Texture
 *
 * Item has an ItemType (ItemTypeId)
 * ItemType has sprite information:
 *  width:     >1 if the full sprite has more than 1 sprite in width
 *  height:    >1 if the full sprite has more than 1 sprite in height
 *  extra:     width and/or height size (instead of 32) depending on width and height
 *  blend:     default 1: no action
 *             if ITEM   and blend=2: blend two sprites together
 *             if OUTFIT and blend=2: sprite is colored based on outfit info
 *             if OTHER  and blend=2: invalid?
 *  xdiv:      if ITEM and not countable: different sprites for different (global) position in x
 *             if ITEM and     countable: 8 sets of sprites for when count is: 1, 2, 3, 4, 5 ... ?
 *             if OUTFIT and 4: 4 sets of sprites, one per direction
 *  ydiv:      different sprites for different (global) position in y
 *  num_anims: number of animations
 *             note: for creatures first anim is standing still, and the rest is walking
 *
 * Total number of sprites: width * height * blend * xdiv * ydiv * num_anim
 *
 * Texture is a "full" sprite, e.g. full width and height
 *
 * Total number of textures: xdiv * ydiv * num_anim
 *
 * Select texture based on global position or creature direction and animation tick
 */
Texture Texture::create(SDL_Renderer* renderer,
                        const io::SpriteLoader& sprite_loader,
                        const common::ItemType& item_type)
{
  Texture texture;
  texture.m_item_type_id = item_type.id;

  // Validate stuff
  // This should probably be validated when the data file is read instead

  // Valid blend value is 1 or 2
  if (item_type.sprite_blend_frames != 1U &&
      item_type.sprite_blend_frames != 2U)
  {
    LOG_ERROR("%s: invalid blend value: %u in item type: %u",
              __func__,
              item_type.sprite_blend_frames,
              item_type.id);
    return texture;
  }

  // blend=2 is only valid for item (blend) and creature (colorize)
  if (item_type.sprite_blend_frames == 2U &&
      item_type.type != common::ItemType::Type::ITEM &&
      item_type.type != common::ItemType::Type::CREATURE)
  {
    LOG_ERROR("%s invalid combination of blend value: 2 and type: %d in item type: %u",
              __func__,
              static_cast<int>(item_type.type),
              item_type.id);
    return texture;
  }

  // creature with xdiv=4 means 4 directions, ydiv should be 1
  if (item_type.type == common::ItemType::Type::CREATURE &&
      item_type.sprite_xdiv == 4U &&
      item_type.sprite_ydiv != 1U)
  {
    LOG_ERROR("%s: invalid combination of CREATURE, xdiv=%u and ydiv=%u (direction)",
              __func__,
              item_type.sprite_xdiv,
              item_type.sprite_ydiv);
    return texture;
  }

  // validate that item that are countable have xdiv=4,ydiv=2?

  const auto directions = item_type.type == common::ItemType::Type::CREATURE &&
                          item_type.sprite_xdiv == 4U;
  const auto blend = item_type.type != common::ItemType::Type::CREATURE &&
                     item_type.sprite_blend_frames == 2U;
  const auto colorize = item_type.type == common::ItemType::Type::CREATURE &&
                        item_type.sprite_blend_frames == 2U;

  const auto num_sprites_per_texture = item_type.sprite_width *
                                       item_type.sprite_height *
                                       (blend || colorize ? 2U : 1U);
  const auto num_textures = item_type.sprite_xdiv *
                            item_type.sprite_ydiv *
                            item_type.sprite_num_anim;
  auto sprite_it = item_type.sprites.begin();
  for (auto i = 0; i < num_textures; i++)
  {
    const auto sprite_ids = std::vector<std::uint16_t>(sprite_it,
                                                       sprite_it + num_sprites_per_texture);
    sprite_it += num_sprites_per_texture;

    SDL_Texture* sdl_texture = nullptr;
    if (!colorize)
    {
      sdl_texture = createSDLTexture(renderer,
                                     sprite_loader,
                                     sprite_ids,
                                     item_type.sprite_width,
                                     item_type.sprite_height,
                                     item_type.sprite_extra,
                                     blend,
                                     directions);
    }
    else
    {
      // TODO
    }

    if (!sdl_texture)
    {
      LOG_ERROR("%s: could not create texture for item type id: %u", __func__, item_type.id);
      texture.m_textures.clear();
      texture.m_texture_ptrs.clear();
      return texture;
    }
    texture.m_textures.emplace_back(sdl_texture, SDL_DestroyTexture);
    texture.m_texture_ptrs.push_back(sdl_texture);
  }

  return texture;
}

}  // namespace wsclient
