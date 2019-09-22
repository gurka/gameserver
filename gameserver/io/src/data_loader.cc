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

#include "data_loader.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include "logger.h"
#include "file_reader.h"

namespace io::data_loader
{

bool load(const std::string& data_filename,
          ItemTypes* item_types,
          world::ItemTypeId* id_first,
          world::ItemTypeId* id_last)
{
  FileReader fr;
  if (!fr.load(data_filename))
  {
    LOG_ERROR("%s: could not open file: %s", __func__, data_filename.c_str());
    return false;
  }

  fr.skip(4);  // skip checksum
  const auto num_items = fr.readU16();
  const auto num_outfits = fr.readU16();
  const auto num_effects = fr.readU16();
  const auto num_missiles = fr.readU16();

  LOG_INFO("%s: num_items: %u num_outfits: %u num_effects: %u, num_missiles: %u",
           __func__,
           num_items,
           num_outfits,
           num_effects,
           num_missiles);

  // 100 is the first item id
  *id_first = 100;
  auto next_id = *id_first;
  for (int i = 0; i < num_items; i++)
  {
    world::ItemType item_type;
    item_type.id = next_id;

    auto opt_byte = fr.readU8();
    while (opt_byte != 0xFFU)
    {
      switch (opt_byte)
      {
        case 0x00:
        {
          // Ground item
          item_type.ground = true;
          item_type.speed = fr.readU8();
          if (item_type.speed == 0)
          {
            item_type.is_blocking = true;
          }
          fr.skip(1);  // TODO(simon): ??
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
          fr.skip(4);
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
        case 0x16:
        case 0x1A:
        {
          // Unknown?
          fr.skip(2);
          break;
        }

        case 0x13:  // render position offset for e.g. boxes, tables, parcels
        {
          item_type.offset = fr.readU16();
          break;
        }

        default:
        {
          LOG_ERROR("%s: Unknown opt_byte: 0x%X", __func__, opt_byte);
          break;
        }
      }

      // Get next optByte
      opt_byte = fr.readU8();
    }

    // Size and sprite data
    item_type.sprite_width = fr.readU8();
    item_type.sprite_height = fr.readU8();
    if (item_type.sprite_width > 1 || item_type.sprite_height > 1)
    {
      item_type.sprite_extra = fr.readU8();;
    }

    item_type.sprite_blend_frames = fr.readU8();
    item_type.sprite_xdiv = fr.readU8();
    item_type.sprite_ydiv = fr.readU8();
    item_type.sprite_num_anim = fr.readU8();

    const auto num_sprites = item_type.sprite_width  *
                             item_type.sprite_height *
                             item_type.sprite_blend_frames *
                             item_type.sprite_xdiv *
                             item_type.sprite_ydiv *
                             item_type.sprite_num_anim;
    for (auto i = 0; i < num_sprites; i++)
    {
      item_type.sprites.push_back(fr.readU16());
    }

    // Add ItemData and increase next item id
    (*item_types)[next_id] = item_type;
    ++next_id;
  }

  *id_last = next_id - 1;

  LOG_INFO("%s: Successfully loaded %d items", __func__, *id_last - *id_first + 1);
  LOG_DEBUG("%s: Last item_id = %d", __func__, *id_last);

  return true;
}

}  // namespace io::data_loader
