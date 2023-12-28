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

#include "utils/logger.h"

/*
 * Item -> ItemType -> Sprites -> Texture
 *
 * Item has an ItemType (ItemTypeId)
 *
 * ItemType has sprite information:
 *  width:     >1 if the full sprite has more than 1 sprite in width
 *  height:    >1 if the full sprite has more than 1 sprite in height
 *  extra:     width and/or height size (instead of 32) depending on width and height
 *  blend:     default 1: no action
 *             if ITEM   and blend=2: blend two sprites together
 *             if OUTFIT and blend=2: sprite is colored based on outfit info
 *             if OTHER  and blend=2: invalid?
 *  xdiv:      different sprites for different (global) position in x
 *             or different sprites for certain items, e.g. countable, hangable, and so on
 *  ydiv:      different sprites for different (global) position in y
 *  num_anims: number of animations
 *             note: for creatures first anim is standing still, and the rest is walking
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

using game::SpritePixels;

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
                            const SpritePixels& sprite_template,
                            const common::Outfit& outfit)
{
  static const std::array<std::uint32_t, 133> lookup_table =
  {
    0xFFFFFF, 0xFFD4BF, 0xFFE9BF, 0xFFFFBF, 0xE9FFBF, 0xD4FFBF, 0xBFFFBF, 0xBFFFD4, 0xBFFFE9, 0xBFFFFF, 0xBFE9FF, 0xBFD4FF, 0xBFBFFF, 0xD4BFFF, 0xE9BFFF, 0xFFBFFF, 0xFFBFE9, 0xFFBFD4, 0xFFBFBF,
    0xDADADA, 0xBF9F8F, 0xBFAF8F, 0xBFBF8F, 0xAFBF8F, 0x9FBF8F, 0x8FBF8F, 0x8FBF9F, 0x8FBFAF, 0x8FBFBF, 0x8FAFBF, 0x8F9FBF, 0x8F8FBF, 0x9F8FBF, 0xAF8FBF, 0xBF8FBF, 0xBF8FAF, 0xBF8F9F, 0xBF8F8F,
    0xB6B6B6, 0xBF7F5F, 0xBFAF8F, 0xBFBF5F, 0x9FBF5F, 0x7FBF5F, 0x5FBF5F, 0x5FBF7F, 0x5FBF9F, 0x5FBFBF, 0x5F9FBF, 0x5F7FBF, 0x5F5FBF, 0x7F5FBF, 0x9F5FBF, 0xBF5FBF, 0xBF5F9F, 0xBF5F7F, 0xBF5F5F,
    0x919191, 0xBF6A3F, 0xBF943F, 0xBFBF3F, 0x94BF3F, 0x6ABF3F, 0x3FBF3F, 0x3FBF6A, 0x3FBF94, 0x3FBFBF, 0x3F94BF, 0x3F6ABF, 0x3F3FBF, 0x6A3FBF, 0x943FBF, 0xBF3FBF, 0xBF3F94, 0xBF3F6A, 0xBF3F3F,
    0x6D6D6D, 0xFF5500, 0xFFAA00, 0xFFFF00, 0xAAFF00, 0x54FF00, 0x00FF00, 0x00FF54, 0x00FFAA, 0x00FFFF, 0x00A9FF, 0x0055FF, 0x0000FF, 0x5500FF, 0xA900FF, 0xFE00FF, 0xFF00AA, 0xFF0055, 0xFF0000,
    0x484848, 0xBF3F00, 0xBF7F00, 0xBFBF00, 0x7FBF00, 0x3FBF00, 0x00BF00, 0x00BF3F, 0x00BF7F, 0x00BFBF, 0x007FBF, 0x003FBF, 0x0000BF, 0x3F00BF, 0x7F00BF, 0xBF00BF, 0xBF007F, 0xBF003F, 0xBF0000,
    0x242424, 0x7F2A00, 0x7F5500, 0x7F7F00, 0x557F00, 0x2A7F00, 0x007F00, 0x007F2A, 0x007F55, 0x007F7F, 0x00547F, 0x002A7F, 0x00007F, 0x2A007F, 0x54007F, 0x7F007F, 0x7F0055, 0x7F002A, 0x7F0000,
  };

  if (outfit.head >= lookup_table.size() ||
      outfit.body >= lookup_table.size() ||
      outfit.legs >= lookup_table.size() ||
      outfit.feet >= lookup_table.size())
  {
    LOG_ERROR("%s: outfit out of bounds for lookup table (head=%u, body=%u, legs=%u, feet=%u)",
              __func__,
              outfit.head,
              outfit.body,
              outfit.legs,
              outfit.feet);
    return sprite_base;
  }

  const auto head_color = lookup_table[outfit.head];
  const auto body_color = lookup_table[outfit.body];
  const auto legs_color = lookup_table[outfit.legs];
  const auto feet_color = lookup_table[outfit.feet];

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
      result[j + 0] *= ((head_color >> 16) & 0xFFU) / 255.f;
      result[j + 1] *= ((head_color >> 8) & 0xFFU) / 255.f;
      result[j + 2] *= ((head_color >> 0) & 0xFFU) / 255.f;
      result[j + 3] = 0xFFU;
    }
    else if (red == 0xFFU && green == 0x00U && blue == 0x00U)
    {
      // Red is body
      result[j + 0] *= ((body_color >> 16) & 0xFFU) / 255.f;
      result[j + 1] *= ((body_color >> 8) & 0xFFU) / 255.f;
      result[j + 2] *= ((body_color >> 0) & 0xFFU) / 255.f;
      result[j + 3] = 0xFFU;
    }
    else if (red == 0x00U && green == 0xFFU && blue == 0x00U)
    {
      // Green is legs
      result[j + 0] *= ((legs_color >> 16) & 0xFFU) / 255.f;
      result[j + 1] *= ((legs_color >> 8) & 0xFFU) / 255.f;
      result[j + 2] *= ((legs_color >> 0) & 0xFFU) / 255.f;
      result[j + 3] = 0xFFU;
    }
    else if (red == 0x00U && green == 0x00U && blue == 0xFFU)
    {
      // Blue is feet
      result[j + 0] *= ((feet_color >> 16) & 0xFFU) / 255.f;
      result[j + 1] *= ((feet_color >> 8) & 0xFFU) / 255.f;
      result[j + 2] *= ((feet_color >> 0) & 0xFFU) / 255.f;
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
                              const common::ItemType::SpriteInfo& sprite_info,
                              const std::vector<SpritePixels>& sprite_data,
                              const common::Outfit& outfit)
{
  const auto blend = sprite_info.shouldBlend();
  const auto colorize = sprite_info.shouldColorize();
  if (blend && colorize)
  {
    LOG_ERROR("%s: both blend and colorize cannot be true", __func__);
    return nullptr;
  }

  // For now, ignore extra and always create the texture either
  // 32x32, 64x32, 32x64 or 64x64
  const auto full_width = sprite_info.width == 1U ? 32U : 64U;
  const auto full_height = sprite_info.height == 1U ? 32U : 64U;

  std::vector<std::uint8_t> texture_pixels(full_width * full_height * 4);
  // If neither blend nor colorize then iterate over all sprites
  // If blend then iterate over half of them (blend with second half)
  // If colorize then iterate over all but with steps of 2
  for (auto i = 0U; i < (sprite_data.size() / (blend ? 2 : 1)); i += (colorize ? 2U : 1U))
  {
    // If neither blend nor colorize then just take sprite data directly
    // If blend then call blendSprites with correct sprite datas
    // If colorize then call colorizeSprite with correct sprite datas
    const auto sprite_pixels = blend ?
                                 blendSprites(sprite_data[i], sprite_data[i + (sprite_info.width * sprite_info.height)]) :
                                 (colorize ?
                                   colorizeSprite(sprite_data[i], sprite_data[i + 1], outfit) :
                                   sprite_data[i]);

    // Hack to treat the two sprites as A and C when width == 1 and height == 2
    if (i == 1 && sprite_info.width == 1 && sprite_info.height == 2)
    {
      i = 2;
    }

    // Where to start writing the pixels on texture_pixels
    const auto start_x = (i == 0 || i == 2) && sprite_info.width == 2 ? 32U : 0U;
    const auto start_y = (i == 0 || i == 1) && sprite_info.height == 2 ? 32U : 0U;

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

namespace game
{

Texture Texture::createOutfitTexture(SDL_Renderer* renderer,
                                     const SpriteLoader& sprite_loader,
                                     const common::ItemType& item_type,
                                     const common::Outfit& outfit)
{
  Texture texture;
  texture.m_item_type = item_type;

  const auto num_sprites_per_texture = item_type.sprite_info.getNumSpritesPerTexture();
  for (auto i = 0; i < item_type.sprite_info.getNumTextures(); i++)
  {
    std::vector<SpritePixels> sprite_data;
    for (auto j = 0; j < num_sprites_per_texture; j++)
    {
      const auto sprite_index = (i * num_sprites_per_texture) + j;
      const auto sprite_id = item_type.sprite_info.sprite_ids[sprite_index];
      sprite_data.push_back(sprite_loader.getSpritePixels(sprite_id));
    }

    auto* sdl_texture = createSDLTexture(renderer, item_type.sprite_info, sprite_data, outfit);
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

Texture Texture::createItemTexture(SDL_Renderer* renderer,
                                   const SpriteLoader& sprite_loader,
                                   const common::ItemType& item_type)
{
  return createOutfitTexture(renderer, sprite_loader, item_type, common::Outfit());
}

SDL_Texture* Texture::getItemTexture(int version, int anim_tick) const
{
  if (version >= getNumVersions())
  {
    LOG_ERROR("%s: version: %d is invalid (getNumVersions(): %d) (item type id: %d)",
              __func__,
              version,
              getNumVersions(),
              m_item_type.id);
    return nullptr;
  }

  const auto index = version + ((anim_tick % getNumAnimations()) * getNumVersions());
  if (index >= static_cast<int>(m_textures.size()))
  {
    LOG_ERROR("%s: calculated index out of bounds (%d >= %d), index = %d + (%d * %d)",
              __func__,
              index,
              m_textures.size(),
              version,
              (anim_tick % getNumAnimations()),
              getNumVersions());
    return nullptr;
  }
  return m_textures[index].get();
}

SDL_Texture* Texture::getCreatureStillTexture(common::Direction direction) const
{
  // Some creatures does not have different sprites based on direction (?)
  if (m_textures.size() == 1U)
  {
    return m_textures[0].get();
  }

  const auto texture_index = static_cast<int>(direction);
  if (texture_index < 0 || texture_index >= getNumTextures())
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
  const auto texture_index =
      static_cast<int>(direction) + (((walk_tick % (m_item_type.sprite_info.getNumAnimations() - 1)) + 1) * 4);
  if (texture_index < 0 || texture_index >= getNumTextures())
  {
    LOG_ERROR("%s: texture_index: %d is invalid (m_textures.size(): %d)",
              __func__,
              texture_index,
              static_cast<int>(m_textures.size()));
    return nullptr;
  }

  return m_textures[texture_index].get();
}

}  // namespace game