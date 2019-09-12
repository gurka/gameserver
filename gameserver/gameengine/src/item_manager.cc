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

#include "item_manager.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include "rapidxml.hpp"
#include "logger.h"

constexpr ItemUniqueId Item::INVALID_UNIQUE_ID;

bool ItemManager::loadItemTypes(const std::string& data_filename, const std::string& items_filename)
{
  if (!loadItemTypesDataFile(data_filename))
  {
    LOG_ERROR("%s: could not load datafile: %s", __func__, data_filename.c_str());
    return false;
  }

  if (!loadItemTypesItemsFile(items_filename))
  {
    LOG_ERROR("%s: could not load itemsfile: %s", __func__, items_filename.c_str());
    return false;
  }

  // dumpItemTypeToJson();

  return true;
}

ItemUniqueId ItemManager::createItem(ItemTypeId item_type_id)
{
  if (item_type_id < m_item_types_id_first || item_type_id > m_item_types_id_last)
  {
    LOG_ERROR("%s: item_type_id: %d out of range", __func__, item_type_id);
    return Item::INVALID_UNIQUE_ID;
  }

  const auto item_unique_id = m_next_item_unique_id;
  ++m_next_item_unique_id;

  m_items.emplace(item_unique_id, ItemImpl(item_unique_id, &m_item_types[item_type_id]));
  LOG_DEBUG("%s: created Item with item_unique_id: %lu, item_type_id: %d", __func__, item_unique_id, item_type_id);

  return item_unique_id;
}

void ItemManager::destroyItem(ItemUniqueId item_unique_id)
{
  if (m_items.count(item_unique_id) == 0)
  {
    LOG_ERROR("%s: could not find Item with item_unique_id: %lu", __func__, item_unique_id);
    return;
  }

  LOG_DEBUG("%s: destroying Item with item_unique_id: %lu", __func__, item_unique_id);
  m_items.erase(item_unique_id);
}

Item* ItemManager::getItem(ItemUniqueId item_unique_id)
{
  if (m_items.count(item_unique_id) == 0)
  {
    return nullptr;
  }

  return &m_items.at(item_unique_id);
}

bool ItemManager::loadItemTypesDataFile(const std::string& data_filename)
{
  // 100 is the first item id
  m_item_types_id_first = 100;
  auto next_item_type_id = m_item_types_id_first;

  // TODO(simon): Use std::ifstream?
  FILE* f = fopen(data_filename.c_str(), "rb");
  if (f == nullptr)
  {
    LOG_ERROR("%s: could not open file: %s", __func__, data_filename.c_str());
    return false;
  }

  fseek(f, 0, SEEK_END);
  auto size = ftell(f);

  fseek(f, 0x0C, SEEK_SET);

  while (ftell(f) < size)
  {
    ItemType item_type;
    item_type.id = next_item_type_id;

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
    m_item_types[next_item_type_id] = item_type;
    ++next_item_type_id;
  }

  m_item_types_id_last = next_item_type_id - 1;

  LOG_INFO("%s: Successfully loaded %d items", __func__, m_item_types_id_last - m_item_types_id_first + 1);
  LOG_DEBUG("%s: Last item_id = %d", __func__, m_item_types_id_last);

  fclose(f);

  return true;
}

bool ItemManager::loadItemTypesItemsFile(const std::string& items_filename)
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
    if (item_id < m_item_types_id_first || item_id > m_item_types_id_last)
    {
      LOG_ERROR("%s: WARNING: Parsed data for Item with id: %d, but that Item does not exist", __func__, item_id);
    }

    auto& item_type = m_item_types[item_id];

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
        item_type.type = attr_value;
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

void ItemManager::dumpItemTypeToJson() const
{
  LOG_INFO(__func__);

  std::ofstream ofs("itemtypes.json");
  ofs << "{\n";
  ofs << "  \"itemTypes\": [\n";
  for (auto id = m_item_types_id_first; id <= m_item_types_id_last; id++)
  {
    const auto& item_type = m_item_types[id];
    ofs << "    { ";

    std::string tmp;

#define VALUE_INT(NAME) if (item_type.NAME != 0) \
                          ofs << "\"" << #NAME << "\": " << item_type.NAME << ", ";
#define VALUE_STR(NAME) if (!item_type.NAME.empty()) \
                          ofs << "\"" << #NAME << "\": \"" << item_type.NAME << "\", ";
#define VALUE_BOOL(NAME) if (item_type.NAME) \
                           ofs << "\"" << #NAME << "\": " << (item_type.NAME ? "true" : "false") << ", ";

    VALUE_INT(id);
    VALUE_BOOL(ground);
    VALUE_INT(speed);
    VALUE_BOOL(is_blocking);
    VALUE_BOOL(always_on_top);
    VALUE_BOOL(is_container);
    VALUE_BOOL(is_stackable);
    VALUE_BOOL(is_usable);
    VALUE_BOOL(is_multitype);
    VALUE_BOOL(is_not_movable);
    VALUE_BOOL(is_equipable);

    VALUE_STR(name);
    VALUE_INT(weight);
    VALUE_INT(decayto);
    VALUE_INT(decaytime);
    VALUE_INT(damage);
    VALUE_INT(maxitems);
    VALUE_STR(type);
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

    if (id != m_item_types_id_last)
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
