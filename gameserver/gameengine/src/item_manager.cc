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

#include "item_manager.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include "rapidxml.hpp"
#include "logger.h"

bool ItemManager::loadItemTypes(const std::string& dataFilename, const std::string& itemsFilename)
{
  if (!loadItemTypesDataFile(dataFilename))
  {
    LOG_ERROR("%s: could not load datafile: %s", __func__, dataFilename.c_str());
    return false;
  }

  if (!loadItemTypesItemsFile(itemsFilename))
  {
    LOG_ERROR("%s: could not load itemsfile: %s", __func__, itemsFilename.c_str());
    return false;
  }

  return true;
}

ItemUniqueId ItemManager::createItem(ItemTypeId itemTypeId)
{
  if (itemTypeId < itemTypesIdFirst_ || itemTypeId > itemTypesIdLast_)
  {
    LOG_ERROR("%s: itemTypeId: %d out of range", __func__, itemTypeId);
    return 0;  // TODO(simon): invalid ItemId (see header and game_position.h)
  }

  const auto itemUniqueId = nextItemUniqueId_;
  ++nextItemUniqueId_;

  items_.emplace(itemUniqueId, ItemImpl(itemUniqueId, &itemTypes_[itemTypeId]));
  LOG_DEBUG("%s: created Item with itemUniqueId: %lu, itemTypeId: %d", __func__, itemUniqueId, itemTypeId);

  return itemUniqueId;
}

void ItemManager::destroyItem(ItemUniqueId itemUniqueId)
{
  if (items_.count(itemUniqueId) == 0)
  {
    LOG_ERROR("%s: could not find Item with itemUniqueId: %lu", __func__, itemUniqueId);
    return;
  }

  LOG_DEBUG("%s: destroying Item with itemUniqueId: %lu", __func__, itemUniqueId);
  items_.erase(itemUniqueId);
}

Item* ItemManager::getItem(ItemUniqueId itemUniqueId)
{
  if (items_.count(itemUniqueId) == 0)
  {
    return nullptr;
  }

  return &items_.at(itemUniqueId);
}

bool ItemManager::loadItemTypesDataFile(const std::string& dataFilename)
{
  // 100 is the first item id
  itemTypesIdFirst_ = 100;
  auto nextItemTypeId = itemTypesIdFirst_;

  // TODO(simon): Use std::ifstream?
  FILE* f = fopen(dataFilename.c_str(), "rb");
  if (f == nullptr)
  {
    LOG_ERROR("%s: could not open file: %s", __func__, dataFilename.c_str());
    return false;
  }

  fseek(f, 0, SEEK_END);
  auto size = ftell(f);

  fseek(f, 0x0C, SEEK_SET);

  while (ftell(f) < size)
  {
    ItemType itemType;
    itemType.id = nextItemTypeId;

    int optByte = fgetc(f);
    while (optByte  >= 0 && optByte != 0xFF)
    {
      switch (optByte)
      {
        case 0x00:
        {
          // Ground item
          itemType.ground = true;
          itemType.speed = fgetc(f);
          if (itemType.speed == 0)
          {
            itemType.isBlocking = true;
          }
          fgetc(f);  // ??
          break;
        }

        case 0x01:
        case 0x02:
        {
          // What's the diff ?
          itemType.alwaysOnTop = true;
          break;
        }

        case 0x03:
        {
          // Container
          itemType.isContainer = true;
          break;
        }

        case 0x04:
        {
          // Stackable
          itemType.isStackable = true;
          break;
        }

        case 0x05:
        {
          // Usable
          itemType.isUsable = true;
          break;
        }

        case 0x0A:
        {
          // Is multitype
          itemType.isMultitype = true;
          break;
        }

        case 0x0B:
        {
          // Blocks
          itemType.isBlocking = true;
          break;
        }

        case 0x0C:
        {
          // Movable
          itemType.isNotMovable = true;
          break;
        }

        case 0x0F:
        {
          // Equipable
          itemType.isEquipable = true;
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
          LOG_ERROR("%s: Unknown optByte: %d", __func__, optByte);
        }
      }

      // Get next optByte
      optByte = fgetc(f);
    }

    // Skip size and sprite data
    auto width = fgetc(f);
    auto height = fgetc(f);
    if (width > 1 || height > 1)
    {
      fgetc(f);  // skip
    }

    auto blendFrames = fgetc(f);
    auto xdiv = fgetc(f);
    auto ydiv = fgetc(f);
    auto animCount = fgetc(f);

    fseek(f, width * height * blendFrames * xdiv * ydiv * animCount * 2, SEEK_CUR);

    // Add ItemData and increase next item id
    itemTypes_[nextItemTypeId] = itemType;
    ++nextItemTypeId;
  }

  itemTypesIdLast_ = nextItemTypeId - 1;

  LOG_INFO("%s: Successfully loaded %d items", __func__, itemTypesIdLast_ - itemTypesIdFirst_ + 1);
  LOG_DEBUG("%s: Last itemId = %d", __func__, itemTypesIdLast_);

  fclose(f);

  return true;
}

bool ItemManager::loadItemTypesItemsFile(const std::string& itemsFilename)
{
  std::ifstream xmlFile(itemsFilename);
  if (!xmlFile.is_open())
  {
    LOG_ERROR("%s: Could not open file: %s", __func__, itemsFilename.c_str());
    return false;
  }

  std::string tempString;
  std::ostringstream xmlStringStream;
  while (std::getline(xmlFile, tempString))
  {
    xmlStringStream << tempString << "\n";
  }

  // Convert the std::string to a char*
  char* xmlString = strdup(xmlStringStream.str().c_str());

  // Parse the XML string with Rapidxml
  rapidxml::xml_document<> itemXml;
  itemXml.parse<0>(xmlString);

  // Get top node (<items>)
  auto* itemsNode = itemXml.first_node("items");
  if (itemsNode == nullptr)
  {
    LOG_ERROR("%s: Invalid file: Could not find node <items>", __func__);
    free(xmlString);
    return false;
  }

  // Iterate over all <item> nodes
  auto numberOfItems = 0;
  for (auto* itemNode = itemsNode->first_node("item"); itemNode != nullptr; itemNode = itemNode->next_sibling())
  {
    numberOfItems++;

    // Get id
    auto* xmlAttrId = itemNode->first_attribute("id");
    if (xmlAttrId == nullptr)
    {
      LOG_ERROR("%s: Invalid file: <item> has no attribute \"id\"", __func__);
      free(xmlString);
      return false;
    }
    auto itemId = std::stoi(xmlAttrId->value());

    // Verify that this item has been loaded
    if (itemId < itemTypesIdFirst_ || itemId > itemTypesIdLast_)
    {
      LOG_ERROR("%s: WARNING: Parsed data for Item with id: %d, but that Item does not exist", __func__, itemId);
    }

    auto& itemType = itemTypes_[itemId];

    // Get name
    auto* xmlAttrName = itemNode->first_attribute("name");
    if (xmlAttrName == nullptr)
    {
      LOG_ERROR("%s: <item>-node has no attribute \"name\"", __func__);
      free(xmlString);
      return false;
    }
    itemType.name = xmlAttrName->value();

    // Iterate over all rest of attributes
    for (auto* xmlAttrOther = itemNode->first_attribute();
         xmlAttrOther != nullptr;
         xmlAttrOther = xmlAttrOther->next_attribute())
    {
      std::string attrName(xmlAttrOther->name());
      if (attrName == "id" || attrName == "name")
      {
        // We have already these attributes
        continue;
      }

      std::string attrValue(xmlAttrOther->value());

      // Handle attributes here
      if (attrName == "weight")
      {
        itemType.weight = std::stoi(attrValue.c_str());
      }
      else if (attrName == "decayto")
      {
        itemType.decayto = std::stoi(attrValue.c_str());
      }
      else if (attrName == "decaytime")
      {
        itemType.decaytime = std::stoi(attrValue.c_str());
      }
      else if (attrName == "damage")
      {
        itemType.damage = std::stoi(attrValue.c_str());
      }
      else if (attrName == "maxitems")
      {
        itemType.maxitems = std::stoi(attrValue.c_str());
      }
      else if (attrName == "type")
      {
        itemType.type = attrValue;
      }
      else if (attrName == "position")
      {
        itemType.position = attrValue;
      }
      else if (attrName == "attack")
      {
        itemType.attack = std::stoi(attrValue.c_str());
      }
      else if (attrName == "defence")
      {
        itemType.defence = std::stoi(attrValue.c_str());
      }
      else if (attrName == "arm")
      {
        itemType.arm = std::stoi(attrValue.c_str());
      }
      else if (attrName == "skill")
      {
        itemType.skill = attrValue;
      }
      else if (attrName == "descr")
      {
        itemType.descr = attrValue;
      }
      else if (attrName == "handed")
      {
        itemType.handed = std::stoi(attrValue.c_str());
      }
      else if (attrName == "shottype")
      {
        itemType.shottype = std::stoi(attrValue.c_str());
      }
      else if (attrName == "amutype")
      {
        itemType.amutype = attrValue;
      }
      else
      {
        LOG_ERROR("%s: unhandled attribute name: %s", __func__, attrName.c_str());
        free(xmlString);
        return false;
      }
    }
  }

  LOG_INFO("%s: Successfully loaded %d items", __func__, numberOfItems);
  free(xmlString);
  return true;
}
