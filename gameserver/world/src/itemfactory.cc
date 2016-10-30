/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
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

#include "itemfactory.h"

#include <cstdio>
#include <fstream>
#include <sstream>

#include "rapidxml.hpp"
#include "logger.h"

bool ItemFactory::initialize(const std::string& dataFilename, const std::string& itemsFilename)
{
  if (!loadFromDat(dataFilename))
  {
    return false;
  }
  if (!loadFromXml(itemsFilename))
  {
    return false;
  }
  return true;
}

bool ItemFactory::loadFromDat(const std::string& dataFilename)
{
  ItemId nextItemId = 100;  // 100 is the first item id

  // TODO(gurka): Use std::ifstream?
  FILE* f = fopen(dataFilename.c_str(), "rb");
  if (f == nullptr)
  {
    LOG_ERROR("loadFromDat(): Could not open file: %s", dataFilename.c_str());
    return false;
  }

  fseek(f, 0, SEEK_END);
  auto size = ftell(f);

  fseek(f, 0x0C, SEEK_SET);

  while (ftell(f) < size)
  {
    ItemData itemData;
    itemData.id = nextItemId;

    int optByte = fgetc(f);
    while (optByte  >= 0 && optByte != 0xFF)
    {
      switch (optByte)
      {
        case 0x00:
        {
          // Ground item
          itemData.ground = true;
          itemData.speed = fgetc(f);
          if (itemData.speed == 0)
          {
            itemData.isBlocking = true;
          }
          fgetc(f);  // ??
          break;
        }

        case 0x01:
        case 0x02:
        {
          // What's the diff ?
          itemData.alwaysOnTop = true;
          break;
        }

        case 0x03:
        {
          // Container
          itemData.isContainer = true;
          break;
        }

        case 0x04:
        {
          // Stackable
          itemData.isStackable = true;
          break;
        }

        case 0x05:
        {
          // Usable
          itemData.isUsable = true;
          break;
        }

        case 0x0A:
        {
          // Is multitype
          itemData.isMultitype = true;
          break;
        }

        case 0x0B:
        {
          // Blocks
          itemData.isBlocking = true;
          break;
        }

        case 0x0C:
        {
          // Movable
          itemData.isNotMovable = true;
          break;
        }

        case 0x0F:
        {
          // Equipable
          itemData.isEquipable = true;
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
          LOG_ERROR("loadItemData: Unknown optByte: %d", optByte);
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

    // Insert ItemData and increase next item id
    itemData_.insert(std::make_pair(itemData.id, itemData));
    nextItemId++;
  }

  LOG_INFO("loadItemData(): Successfully loaded %d items", itemData_.size());
  LOG_DEBUG("loadItemData(): Last itemId = %d", nextItemId - 1);

  fclose(f);

  return true;
}

bool ItemFactory::loadFromXml(const std::string& itemsFilename)
{
  // Open XML and read into string
  std::ifstream xmlFile(itemsFilename);
  if (!xmlFile.is_open())
  {
    LOG_ERROR("loadFromXml(): Could not open file: \"%s\"", itemsFilename.c_str());
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
  rapidxml::xml_node<>* itemsNode = itemXml.first_node("items");
  if (itemsNode == nullptr)
  {
    LOG_ERROR("loadFromXml(): Invalid file: Could not find node <items>");
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
      LOG_ERROR("loadFromXml(): Invalid file: <item> has no attribute \"id\"");
      free(xmlString);
      return false;
    }
    ItemId itemId = std::stoi(xmlAttrId->value());

    // Fetch the ItemData
    if (itemData_.count(itemId) == 0)
    {
      LOG_ERROR("loadFromXml(): Parsed data for Item with id: %d, but that Item does not exist", itemId);
      free(xmlString);
      return false;
    }
    ItemData& itemData = itemData_.at(itemId);

    // Get name
    auto* xmlAttrName = itemNode->first_attribute("name");
    if (xmlAttrName == nullptr)
    {
      LOG_ERROR("loadFromXml(): <item>-node has no attribute \"name\"");
      free(xmlString);
      return false;
    }
    itemData.name = xmlAttrName->value();

    // Iterate over all rest of attributes
    for (auto* xmlAttrOther = itemNode->first_attribute(); xmlAttrOther != nullptr; xmlAttrOther = xmlAttrOther->next_attribute())
    {
      std::string attrName(xmlAttrOther->name());
      if (attrName == "id" || attrName == "name")
      {
        // We have already these attributes
        continue;
      }

      std::string attrValue(xmlAttrOther->value());

      itemData.attributes.insert(std::make_pair(attrName, attrValue));
    }
  }

  LOG_INFO("loadFromXml(): Successfully loaded %d items", numberOfItems);

  free(xmlString);
  return true;
}

Item ItemFactory::createItem(ItemId itemId) const
{
  if (itemData_.count(itemId) == 1)
  {
    return Item(&itemData_.at(itemId));
  }
  else
  {
    return Item(nullptr);  // invalid Item
  }
}
