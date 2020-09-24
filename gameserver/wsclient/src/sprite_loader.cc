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

#include "sprite_loader.h"

#include <cstdint>
#include <array>
#include <fstream>
#include <vector>

#include "logger.h"
#include "file_reader.h"

namespace wsclient
{

SpriteLoader::SpriteLoader()
    : m_fr(new utils::FileReader)
{
}

SpriteLoader::~SpriteLoader() = default;

bool SpriteLoader::load(const std::string& filename)
{
  if (!m_fr->load(filename))
  {
    LOG_ERROR("%s: could not open file: %s", __func__, filename.c_str());
    return false;
  }

  const auto checksum = m_fr->readU32();
  LOG_INFO("%s: checksum: 0x%x", __func__, checksum);

  const auto num_sprites = m_fr->readU16();
  LOG_INFO("%s: number of sprites: %d", __func__, num_sprites);

  for (auto i = 0; i < num_sprites; i++)
  {
    const auto offset = m_fr->readU32();
    m_offsets.push_back(offset);
  }

  return true;
}

SpritePixels SpriteLoader::getSpritePixels(int sprite_id) const
{
  SpritePixels sprite_pixels = {};

  // 0 is full alpha and OK to request, it seems
  if (sprite_id == 0)
  {
    return sprite_pixels;
  }

  // Because reasons
  sprite_id -= 1;

  if (sprite_id < 0 || sprite_id >= static_cast<int>(m_offsets.size()))
  {
    LOG_ERROR("%s: sprite_id: %d is out of bounds", __func__, sprite_id);
    return sprite_pixels;
  }


  const auto offset = m_offsets[sprite_id];
  if (offset == 0)
  {
    LOG_DEBUG("%s: sprite_id: %d is empty", __func__, sprite_id);
    return sprite_pixels;
  }

  // Go to offset + skip 3 first bytes (color key?)
  m_fr->set(offset + 3);

  const auto bytes_to_read = m_fr->readU16();
  auto bytes_read = 0U;
  auto pixel_index = 0;
  while (bytes_read < bytes_to_read)
  {
    const auto num_transparent = m_fr->readU16();
    bytes_read += 2;
    pixel_index += (4 * num_transparent);

    if (bytes_read >= bytes_to_read)
    {
      break;
    }

    const auto num_pixels = m_fr->readU16();
    bytes_read += 2;
    if (bytes_read >= bytes_to_read && num_pixels > 0)
    {
      LOG_ERROR("%s: num_pixels: %d but we have read all bytes...", __func__, num_pixels);
      return sprite_pixels;
    }
    for (int i = 0; i < num_pixels; i++)
    {
      sprite_pixels[pixel_index++] = m_fr->readU8();  // red
      sprite_pixels[pixel_index++] = m_fr->readU8();  // green
      sprite_pixels[pixel_index++] = m_fr->readU8();  // blue
      sprite_pixels[pixel_index++] = 0xFF;
      bytes_read += 3;
    }
  }

  return sprite_pixels;
}

}  // namespace wsclient
