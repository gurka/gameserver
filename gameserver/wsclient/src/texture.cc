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

#include "texture.h"

#include "logger.h"

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
 *
 *  Combinations:
 *
 *  width == 1 && height == 1 (32 x 32):
 *     A
 *
 *  width == 2 && height == 1 (extra x 32):
 *    BA
 *
 *  width == 1 && height == 2 (32 x extra): (see hack below)
 *     C
 *     A
 *
 *  width == 2 && height == 2 (extra x extra):
 *    DC
 *    BA
 *
 * Where the sprite ids are in order: A, B, C, D.
 * If blend is 2 the order is: A1, B1, C1, D1, A2, B2, C2, D2 - blend A1..D1 with A2..D2
 */
namespace
{

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
constexpr auto RMASK = 0xFF000000U;
constexpr auto GMASK = 0x00FF0000U;
constexpr auto BMASK = 0x0000FF00U;
constexpr auto AMASK = 0x000000FFU;
#else
constexpr auto RMASK = 0x000000FFU;
constexpr auto GMASK = 0x0000FF00U;
constexpr auto BMASK = 0x00FF0000U;
constexpr auto AMASK = 0xFF000000U;
#endif

using wsclient::SpritePixels;

SpritePixels blendSprites(const SpritePixels& bottom,
                          const SpritePixels& top)
{
  SpritePixels result = bottom;
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

SpritePixels colorizeSprite(const SpritePixels& sprite_base,
                            const SpritePixels& sprite_template)
{
  SpritePixels result = sprite_base;
  for (auto j = 0U; j < result.size(); j += 4)
  {
    const auto alpha = sprite_template[j + 3];
    if (alpha == 0x00U)
    {
      continue;  // Alpha -> skip
    }

    // Check template if this is a colorize pixel
    const auto red = sprite_template[j + 0];
    const auto green = sprite_template[j + 1];
    const auto blue = sprite_template[j + 2];
    if (red == 0xFFU && green == 0xFFU && blue == 0x00U)
    {
      // Yellow is head
      result[j + 0] = (result[j + 0] + 120U) / 2U;
      result[j + 1] = (result[j + 1] + 61U) / 2U;
      result[j + 2] = (result[j + 2] + 10U) / 2U;
      result[j + 3] = 0xFFU;
    }
    else if (red == 0xFFU && green == 0x00U && blue == 0x00U)
    {
      // Red is body
      result[j + 0] = (result[j + 0] + 255U) / 2U;
      result[j + 1] = (result[j + 1] + 135U) / 2U;
      result[j + 2] = (result[j + 2] + 221U) / 2U;
      result[j + 3] = 0xFFU;
    }
    else if (red == 0x00U && green == 0xFFU && blue == 0x00U)
    {
      // Green is legs
      result[j + 0] = (result[j + 0] + 23U) / 2U;
      result[j + 1] = (result[j + 1] + 60U) / 2U;
      result[j + 2] = (result[j + 2] + 128U) / 2U;
      result[j + 3] = 0xFFU;
    }
    else if (red == 0x00U && green == 0x00U && blue == 0xFFU)
    {
      // Blue is feet
      result[j + 0] = (result[j + 0] + 99U) / 2U;
      result[j + 1] = (result[j + 1] + 99U) / 2U;
      result[j + 2] = (result[j + 2] + 99U) / 2U;
      result[j + 3] = 0xFFU;
    }
    else
    {
      LOG_ERROR("%s: invalid pixel in template: r=%u g=%u b=%u a=%u",
                __func__,
                red,
                green,
                blue,
                alpha);
    }
  }
  return result;
}
SDL_Texture* createSDLTexture(SDL_Renderer* renderer,
                              const std::vector<SpritePixels>& sprite_data,
                              std::uint8_t width,
                              std::uint8_t height,
                              std::uint8_t extra,
                              bool blend,
                              bool colorize)
{
  if (blend && colorize)
  {
    LOG_ERROR("%s: both blend and colorize can be true", __func__);
    return nullptr;
  }

  // For now, ignore extra and always create the texture either
  // 32x32, 64x32, 32x64 or 64x64
  extra = 64U;

  const auto full_width = width == 1U ? 32U : extra;
  const auto full_height = height == 1U ? 32U : extra;

  // Validate number of sprites
  if (width * height * ((blend || colorize) ? 2 : 1) != sprite_data.size())
  {
    LOG_ERROR("%s: unexpected num sprites: %u with width: %u height: %u blend: %d colorize: %d",
              __func__,
              sprite_data.size(),
              width,
              height,
              blend,
              colorize);
    return nullptr;
  }

  std::vector<std::uint8_t> texture_pixels(full_width * full_height * 4);
  // If neither blend nor colorize then iterate over all sprites
  // If blend then iterate over half of them (blend with second half)
  // If colorize then iterate over all but with steps of 2
  for (auto i = 0U; i < (sprite_data.size() / (blend ? 2 : 1)); i += (colorize ? 2U : 1U))
  {
    // If neither blend nor colorize then just take sprite data directly
    // If blend then call blendSprites with correct sprite datas
    // If colorize then call colorizeSprite with correct sprite datas
    const auto sprite_pixels = blend ? blendSprites(sprite_data[i],
                                                    sprite_data[i + (width * height)])
                                     : (colorize ? colorizeSprite(sprite_data[i],
                                                                  sprite_data[i + 1])
                                                 : sprite_data[i]);

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
                                           RMASK,
                                           GMASK,
                                           BMASK,
                                           AMASK);
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

Texture Texture::create(SDL_Renderer* renderer,
                        const SpriteLoader& sprite_loader,
                        const common::ItemType& item_type)
{
  Texture texture;
  texture.m_item_type = item_type;

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
  for (auto i = 0; i < num_textures; i++)
  {
    std::vector<SpritePixels> sprite_data;
    for (auto j = 0U; j < num_sprites_per_texture; j++)
    {
      const auto sprite_index = (i * num_sprites_per_texture) + j;
      const auto sprite_id = item_type.sprites[sprite_index];
      sprite_data.push_back(sprite_loader.getSpritePixels(sprite_id));
    }

    auto* sdl_texture = createSDLTexture(renderer,
                                         sprite_data,
                                         item_type.sprite_width,
                                         item_type.sprite_height,
                                         item_type.sprite_extra,
                                         blend,
                                         colorize);
    if (!sdl_texture)
    {
      LOG_ERROR("%s: could not create texture for item type id: %u", __func__, item_type.id);
      texture.m_textures.clear();
      return texture;
    }
    texture.m_textures.emplace_back(sdl_texture, SDL_DestroyTexture);
  }

  return texture;
}

SDL_Texture* Texture::getItemTexture(const common::Position& position, int anim_tick) const
{
  // TODO(simon): this isn't correct, x or anim_tick need a multiplier as well
  const auto texture_index = (position.getX() % m_item_type.sprite_xdiv) +
                             ((position.getY() % m_item_type.sprite_ydiv) * m_item_type.sprite_xdiv) +
                             (anim_tick % m_item_type.sprite_num_anim);
  if (texture_index < 0 || texture_index >= static_cast<int>(m_textures.size()))
  {
    LOG_ERROR("%s: texture_index: %d is invalid (m_textures.size(): %d)",
              __func__,
              texture_index,
              static_cast<int>(m_textures.size()));
    return nullptr;
  }

  return m_textures[texture_index].get();
}

SDL_Texture* Texture::getCreatureStillTexture(common::Direction direction) const
{
  // Some creatures does not have different sprites based on direction (?)
  if (m_textures.size() == 1U)
  {
    return m_textures[0].get();
  }

  const auto texture_index = static_cast<int>(direction);
  if (texture_index < 0 || texture_index >= static_cast<int>(m_textures.size()))
  {
    LOG_ERROR("%s: texture_index: %d is invalid (m_textures.size(): %d)",
              __func__,
              texture_index,
              static_cast<int>(m_textures.size()));
    return nullptr;
  }

  return m_textures[texture_index].get();
}

SDL_Texture* Texture::getCreatureWalkTexture(common::Direction direction, int walk_tick) const
{
  const auto texture_index = static_cast<int>(direction) + (((walk_tick % (m_item_type.sprite_num_anim - 1)) + 1) * 4);
  if (texture_index < 0 || texture_index >= static_cast<int>(m_textures.size()))
  {
    LOG_ERROR("%s: texture_index: %d is invalid (m_textures.size(): %d)",
              __func__,
              texture_index,
              static_cast<int>(m_textures.size()));
    return nullptr;
  }

  return m_textures[texture_index].get();
}

}  // namespace wsclient
