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
#include "sprite_loader.h"

#include <cstdint>
#include <array>
#include <fstream>
#include <vector>

#include <SDL/SDL.h>

#include "logger.h"

namespace wsclient::sprite
{

bool Reader::load(const std::string& filename)
{
  if (!m_fr.load(filename))
  {
    LOG_ERROR("%s: could not open file: %s", __func__, filename.c_str());
    return false;
  }

  const auto checksum = m_fr.readU32();
  LOG_INFO("%s: checksum: 0x%x", __func__, checksum);

  const auto num_sprites = m_fr.readU16();
  LOG_INFO("%s: number of sprites: %d", __func__, num_sprites);

  for (auto i = 0; i < num_sprites; i++)
  {
    const auto offset = m_fr.readU32();
    m_offsets.push_back(offset);
  }

  return true;
}

SDL_Texture* Reader::get_sprite(int sprite_id, SDL_Renderer* renderer)
{
  sprite_id -= 1;

  if (sprite_id < 0 || sprite_id >= static_cast<int>(m_offsets.size()))
  {
    LOG_ERROR("%s: sprite_id: %d is out of bounds", __func__, sprite_id);
    return nullptr;
  }

  // Check cache
  const auto it = std::find_if(m_textures.cbegin(), m_textures.cend(), [&sprite_id](const TextureCache& tc)
  {
    return tc.sprite_id == sprite_id;
  });

  if (it != m_textures.cend())
  {
    return it->texture;
  }

  const auto offset = m_offsets[sprite_id];
  if (offset == 0)
  {
    // TODO: return empty texture
    LOG_ERROR("%s: sprite_id: %d is empty", __func__, sprite_id);
    return nullptr;
  }

  // Skip 3 first bytes (color key?)
  m_fr.set(offset + 3);

  const auto bytes_to_read = m_fr.readU16();
  auto bytes_read = 0U;
  std::array<std::uint8_t, 32 * 32 * 4> pixel_data {};
  auto pixel_index = 0;
  while (bytes_read < bytes_to_read)
  {
    const auto num_transparent = m_fr.readU16();
    bytes_read += 2;
    pixel_index += (4 * num_transparent);

    if (bytes_read >= bytes_to_read)
    {
      break;
    }

    const auto num_pixels = m_fr.readU16();
    bytes_read += 2;
    if (bytes_read >= bytes_to_read && num_pixels > 0)
    {
      LOG_ERROR("%s: num_pixels: %d but we have read all bytes...", __func__, num_pixels);
      return nullptr;
    }
    for (int i = 0; i < num_pixels; i++)
    {
      pixel_data[pixel_index++] = m_fr.readU8();  // red
      pixel_data[pixel_index++] = m_fr.readU8();  // green
      pixel_data[pixel_index++] = m_fr.readU8();  // blue
      pixel_data[pixel_index++] = 0xFF;
      bytes_read += 3;
    }
  }

  // Create surface from pixels
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
  auto* surface = SDL_CreateRGBSurfaceFrom(pixel_data.data(), 32, 32, 32, 32 * 4, rmask, gmask, bmask, amask);
  if (!surface)
  {
    LOG_ERROR("%s: could not create surface: %s", __func__, SDL_GetError());
    return nullptr;
  }

  // Create texture from surface
  auto* texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture)
  {
    LOG_ERROR("%s: could not create texture: %s", __func__, SDL_GetError());
    return nullptr;
  }

  SDL_FreeSurface(surface);

  // Add to cache
  m_textures.push_back(TextureCache{sprite_id, texture});

  return texture;
}

}  // namespace wsclient::sprite_loader
