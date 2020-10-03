/* The MIT License (MIT)
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

#include "data_loader.h"

#include <cstring>
#include <fstream>
#include <sstream>

#include "logger.h"
#include "file_reader.h"
#include "rapidxml.hpp"

namespace utils::data_loader
{

bool load(const std::string& data_filename,
          ItemTypes* item_types,
          common::ItemTypeId* id_first,
          common::ItemTypeId* id_last)
{
  // Allow nullptr id_first and id_last, but then we need to use
  // internal variables
  common::ItemTypeId id_first_internal;
  if (!id_first)
  {
    id_first = &id_first_internal;
  }
  common::ItemTypeId id_last_internal;
  if (!id_last)
  {
    id_last = &id_last_internal;
  }

  FileReader fr;
  if (!fr.load(data_filename))
  {
    LOG_ERROR("%s: could not open file: %s", __func__, data_filename.c_str());
    return false;
  }

  fr.skip(4);  // skip checksum
  const auto num_items = fr.readU16() - 99; // id 0..99 is invalid
  const auto num_outfits = fr.readU16();
  const auto num_effects = fr.readU16();
  const auto num_missiles = fr.readU16();
  const auto num_total = num_items + num_outfits + num_effects + num_missiles;

  LOG_INFO("%s: num_items: %u num_outfits: %u num_effects: %u, num_missiles: %u, num_total: %u",
           __func__,
           num_items,
           num_outfits,
           num_effects,
           num_missiles,
           num_total);

  // 100 is the first item id
  *id_first = 100;
  auto next_id = *id_first;
  for (int i = 0; i < num_total; i++)
  {
    common::ItemType item_type;
    item_type.id = next_id;

    if (next_id - *id_first < num_items)
    {
      item_type.type = common::ItemType::Type::ITEM;
    }
    else if (next_id - *id_first - num_items < num_outfits)
    {
      item_type.type = common::ItemType::Type::CREATURE;
    }
    else if (next_id - *id_first - num_items - num_outfits < num_effects)
    {
      item_type.type = common::ItemType::Type::EFFECT;
    }
    else  // assume it's a missile
    {
      item_type.type = common::ItemType::Type::MISSILE;
    }

    auto opt_byte = fr.readU8();
    while (opt_byte != 0xFFU)
    {
      switch (opt_byte)
      {
        case 0x00:
          item_type.is_ground = true;
          item_type.speed = fr.readU16();
          break;

        case 0x01:
          item_type.is_on_bottom = true;
          break;

        case 0x02:
          item_type.is_on_top = true;
          break;

        case 0x03:
          item_type.is_container = true;
          break;

        case 0x04:
          item_type.is_stackable = true;
          break;

        case 0x05:
          item_type.is_multi_use = true;
          break;

        case 0x06:
          item_type.is_force_use = true;
          break;

        case 0x07:
          item_type.is_writable = true;
          item_type.writable_length = fr.readU16();
          break;

        case 0x08:
          item_type.is_writable_once = true;
          item_type.writable_length = fr.readU16();
          break;

        case 0x09:
          item_type.is_fluid_container = true;
          break;

        case 0x0A:
          item_type.is_splash = true;
          break;

        case 0x0B:
          item_type.is_blocking = true;
          break;

        case 0x0C:
          item_type.is_immovable = true;
          break;

        case 0x0D:
          item_type.is_missile_block = true;
          break;

        case 0x0E:
          item_type.is_not_pathable = true;
          break;

        case 0x0F:
          item_type.is_equipable = true;
          break;

        case 0x10:
          item_type.light_size = fr.readU8();
          item_type.light_data[0] = fr.readU8();
          item_type.light_data[1] = fr.readU8();
          item_type.light_data[2] = fr.readU8();
          break;

        case 0x11:
          item_type.is_floor_change = true;
          break;

        case 0x12:
          item_type.is_full_ground = true;
          break;

        case 0x13:
          item_type.elevation = fr.readU16();
          break;

        case 0x14:
          item_type.is_displaced = true;
          break;

        // no 0x15?

        case 0x16:
          item_type.minimap_color = fr.readU16();
          break;

        case 0x17:
          item_type.is_rotateable = true;
          break;

        case 0x18:
          item_type.is_corpse = true;
          break;

        case 0x19:
          item_type.is_hangable = true;
          break;

        case 0x1A:
          item_type.is_hook_south = true;
          break;

        case 0x1B:
          item_type.is_hook_east = true;
          break;

        case 0x1C:
          item_type.is_animate_always = true;
          break;

        case 0x1D:
          fr.readU16();  // lens help -> ignore
          break;

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
  LOG_DEBUG("%s: Last item_id = %d (file offset = %d)", __func__, *id_last, fr.offset());

  return true;
}

bool loadXml(const std::string& items_filename,
             ItemTypes* item_types,
             common::ItemTypeId id_first,
             common::ItemTypeId id_last)
{
  std::ifstream xml_file(items_filename);
  if (!xml_file.is_open())
  {
    LOG_ERROR("%s: Could not open file: %s", __func__, items_filename.c_str());
    return false;
  }

  std::string tmp_string;
  std::ostringstream xml_stream;
  while (std::getline(xml_file, tmp_string))
  {
    xml_stream << tmp_string << "\n";
  }

  // Convert the std::string to a char*
  char* xml_string = strdup(xml_stream.str().c_str());

  // Parse the XML string with Rapidxml
  rapidxml::xml_document<> item_xml;
  item_xml.parse<0>(xml_string);

  // Get top node (<items>)
  auto* items_node = item_xml.first_node("items");
  if (items_node == nullptr)
  {
    LOG_ERROR("%s: Invalid file: Could not find node <items>", __func__);
    free(xml_string);
    return false;
  }

  // Iterate over all <item> nodes
  auto num_items = 0;
  for (auto* item_node = items_node->first_node("item"); item_node != nullptr; item_node = item_node->next_sibling())
  {
    num_items++;

    // Get id
    auto* xml_attr_id = item_node->first_attribute("id");
    if (xml_attr_id == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <item> has no attribute \"id\"", __func__);
      free(xml_string);
      return false;
    }
    auto item_id = std::stoi(xml_attr_id->value());

    // Verify that this item has been loaded
    if (item_id < id_first || item_id > id_last)
    {
      LOG_ERROR("%s: WARNING: Parsed data for Item with id: %d, but that Item does not exist", __func__, item_id);
    }

    auto& item_type = (*item_types)[item_id];

    // Get name
    auto* xml_attr_name = item_node->first_attribute("name");
    if (xml_attr_name == nullptr)
    {
      LOG_ERROR("%s: <item>-node has no attribute \"name\"", __func__);
      free(xml_string);
      return false;
    }
    item_type.name = xml_attr_name->value();

    // Iterate over all rest of attributes
    for (auto* xml_attr_other = item_node->first_attribute();
         xml_attr_other != nullptr;
         xml_attr_other = xml_attr_other->next_attribute())
    {
      std::string attr_name(xml_attr_other->name());
      if (attr_name == "id" || attr_name == "name")
      {
        // We have already these attributes
        continue;
      }

      std::string attr_value(xml_attr_other->value());

      // Handle attributes here
      if (attr_name == "weight")
      {
        item_type.weight = std::stoi(attr_value);
      }
      else if (attr_name == "decayto")
      {
        item_type.decayto = std::stoi(attr_value);
      }
      else if (attr_name == "decaytime")
      {
        item_type.decaytime = std::stoi(attr_value);
      }
      else if (attr_name == "damage")
      {
        item_type.damage = std::stoi(attr_value);
      }
      else if (attr_name == "maxitems")
      {
        item_type.maxitems = std::stoi(attr_value);
      }
      else if (attr_name == "type")
      {
        item_type.type_xml = attr_value;
      }
      else if (attr_name == "position")
      {
        item_type.position = attr_value;
      }
      else if (attr_name == "attack")
      {
        item_type.attack = std::stoi(attr_value);
      }
      else if (attr_name == "defence")
      {
        item_type.defence = std::stoi(attr_value);
      }
      else if (attr_name == "arm")
      {
        item_type.arm = std::stoi(attr_value);
      }
      else if (attr_name == "skill")
      {
        item_type.skill = attr_value;
      }
      else if (attr_name == "descr")
      {
        item_type.descr = attr_value;
      }
      else if (attr_name == "handed")
      {
        item_type.handed = std::stoi(attr_value);
      }
      else if (attr_name == "shottype")
      {
        item_type.shottype = std::stoi(attr_value);
      }
      else if (attr_name == "amutype")
      {
        item_type.amutype = attr_value;
      }
      else
      {
        LOG_ERROR("%s: unhandled attribute name: %s", __func__, attr_name.c_str());
        free(xml_string);
        return false;
      }
    }
  }

  LOG_INFO("%s: Successfully loaded %d items", __func__, num_items);
  free(xml_string);
  return true;
}

void dumpToJson(const ItemTypes& item_types,
                common::ItemTypeId id_first,
                common::ItemTypeId id_last)
{
  LOG_INFO(__func__);

  std::ofstream ofs("itemtypes.json");
  ofs << "{\n";
  ofs << "  \"itemTypes\": [\n";
  for (auto id = id_first; id <= id_last; id++)
  {
    const auto& item_type = item_types[id];
    ofs << "    { ";

    std::string tmp;

#define VALUE_INT(NAME) if (item_type.NAME != 0) \
                          ofs << "\"" << #NAME << "\": " << item_type.NAME << ", ";
#define VALUE_STR(NAME) if (!item_type.NAME.empty()) \
                          ofs << "\"" << #NAME << "\": \"" << item_type.NAME << "\", ";
#define VALUE_BOOL(NAME) if (item_type.NAME) \
                           ofs << "\"" << #NAME << "\": " << (item_type.NAME ? "true" : "false") << ", ";

    VALUE_INT(id);

    VALUE_BOOL(is_ground);
    VALUE_BOOL(is_on_bottom);
    VALUE_BOOL(is_on_top);
    VALUE_BOOL(is_container);
    VALUE_BOOL(is_stackable);
    VALUE_BOOL(is_multi_use);
    VALUE_BOOL(is_force_use);
    VALUE_BOOL(is_writable);
    VALUE_BOOL(is_writable_once);
    VALUE_BOOL(is_fluid_container)
    VALUE_BOOL(is_splash);
    VALUE_BOOL(is_blocking);
    VALUE_BOOL(is_immovable);
    VALUE_BOOL(is_missile_block);
    VALUE_BOOL(is_not_pathable);
    VALUE_BOOL(is_equipable);
    VALUE_BOOL(is_floor_change);
    VALUE_BOOL(is_full_ground);
    VALUE_BOOL(is_displaced);
    VALUE_BOOL(is_rotateable);
    VALUE_BOOL(is_corpse);
    VALUE_BOOL(is_hangable);
    VALUE_BOOL(is_hook_south);
    VALUE_BOOL(is_hook_east);
    VALUE_BOOL(is_animate_always);

    VALUE_INT(speed);
    VALUE_INT(writable_length);
    VALUE_INT(light_size);
    VALUE_INT(light_data[0]);
    VALUE_INT(light_data[1]);
    VALUE_INT(light_data[2]);
    VALUE_INT(elevation);
    VALUE_INT(minimap_color);

    VALUE_STR(name);
    VALUE_INT(weight);
    VALUE_INT(decayto);
    VALUE_INT(decaytime);
    VALUE_INT(damage);
    VALUE_INT(maxitems);
    VALUE_STR(type_xml);
    VALUE_STR(position);
    VALUE_INT(attack);
    VALUE_INT(defence);
    VALUE_INT(arm);
    VALUE_STR(skill);
    VALUE_STR(descr);
    VALUE_INT(handed);
    VALUE_INT(shottype);
    VALUE_STR(amutype);

#undef VALUE_INT
#undef VALUE_STR
#undef VALUE_BOOL

    // Go back two characters
    ofs.seekp(-2, std::ofstream::cur);
    ofs << " }";

    if (id != id_last)
    {
      ofs << ",\n";
    }
    else
    {
      ofs << "\n";
    }
  }
  ofs << "  ]\n";
  ofs << "}\n";

  LOG_INFO("%s: done", __func__);
}

}  // namespace utils::data_loader
