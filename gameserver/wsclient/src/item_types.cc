/* The MIT License (MIT)
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

#include "item_types.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include "logger.h"

namespace wsclient::item_types
{

std::optional<wsworld::ItemTypes> load(const std::string& data_filename)
{
  wsworld::ItemTypes item_types;

  // 100 is the first item id
  const auto id_first = 100;
  auto next_id = id_first;

  // TODO(simon): Use std::ifstream?
  FILE* f = fopen(data_filename.c_str(), "rb");
  if (f == nullptr)
  {
    LOG_ERROR("%s: could not open file: %s", __func__, data_filename.c_str());
    return {};
  }

  fseek(f, 0, SEEK_END);
  auto size = ftell(f);

  fseek(f, 0x0C, SEEK_SET);

  while (ftell(f) < size)
  {
    wsworld::ItemType item_type;
    item_type.id = next_id;

    auto opt_byte = fgetc(f);
    while (opt_byte  >= 0 && opt_byte != 0xFF)
    {
      switch (opt_byte)
      {
        case 0x00:
        {
          // Ground item
          item_type.ground = true;
          item_type.speed = fgetc(f);
          if (item_type.speed == 0)
          {
            item_type.is_blocking = true;
          }
          fgetc(f);  // ??
          break;
        }

        case 0x01:
        case 0x02:
        {
          // What's the diff ?
          item_type.always_on_top = true;
          break;
        }

        case 0x03:
        {
          // Container
          item_type.is_container = true;
          break;
        }

        case 0x04:
        {
          // Stackable
          item_type.is_stackable = true;
          break;
        }

        case 0x05:
        {
          // Usable
          item_type.is_usable = true;
          break;
        }

        case 0x0A:
        {
          // Is multitype
          item_type.is_multitype = true;
          break;
        }

        case 0x0B:
        {
          // Blocks
          item_type.is_blocking = true;
          break;
        }

        case 0x0C:
        {
          // Movable
          item_type.is_not_movable = true;
          break;
        }

        case 0x0F:
        {
          // Equipable
          item_type.is_equipable = true;
          break;
        }

        case 0x10:
        {
          // Makes light (skip 4 bytes)
          fseek(f, 4, SEEK_CUR);
          break;
        }

        case 0x06:
        case 0x09:
        case 0x0D:
        case 0x0E:
        case 0x11:
        case 0x12:
        case 0x14:
        case 0x18:
        case 0x19:
        {
          // Unknown?
          break;
        }

        case 0x07:
        case 0x08:
        case 0x13:
        case 0x16:
        case 0x1A:
        {
          // Unknown?
          fseek(f, 2, SEEK_CUR);
          break;
        }

        default:
        {
          LOG_ERROR("%s: Unknown opt_byte: %d", __func__, opt_byte);
        }
      }

      // Get next optByte
      opt_byte = fgetc(f);
    }

    // Skip size and sprite data
    const auto width = fgetc(f);
    const auto height = fgetc(f);
    if (width > 1 || height > 1)
    {
      fgetc(f);  // skip
    }

    const auto blend_frames = fgetc(f);
    const auto xdiv = fgetc(f);
    const auto ydiv = fgetc(f);
    const auto anim_count = fgetc(f);

    fseek(f, width * height * blend_frames * xdiv * ydiv * anim_count * 2, SEEK_CUR);

    // Add ItemData and increase next item id
    item_types[next_id] = item_type;
    ++next_id;
  }

  const auto id_last = next_id - 1;

  LOG_INFO("%s: Successfully loaded %d items", __func__, id_last - id_first + 1);
  LOG_DEBUG("%s: Last item_id = %d", __func__, id_last);

  fclose(f);

  return item_types;
}

}  // namespace wsclient::item_types
