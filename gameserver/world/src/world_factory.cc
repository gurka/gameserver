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

#include "world_factory.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "position.h"
#include "tile.h"
#include "item_factory.h"
#include "world.h"
#include "logger.h"
#include "rapidxml.hpp"

std::unique_ptr<World> WorldFactory::createWorld(const std::string& dataFilename,
                                                 const std::string& itemsFilename,
                                                 const std::string& worldFilename)
{
  // Load ItemFactory
  auto itemFactory = std::unique_ptr<ItemFactory>(new ItemFactory());
  if (!itemFactory->initialize(dataFilename, itemsFilename))
  {
    LOG_ERROR("%s: Could not initialize ItemFactory", __func__);
    return std::unique_ptr<World>();
  }

  // Open world.xml and read it into a string
  LOG_INFO("Loading world file: \"%s\"", worldFilename.c_str());
  std::ifstream xmlFile(worldFilename);
  if (!xmlFile.is_open())
  {
    LOG_ERROR("%s: Could not open file: \"%s\"", __func__, worldFilename.c_str());
    return std::unique_ptr<World>();
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
  rapidxml::xml_document<> worldXml;
  worldXml.parse<0>(xmlString);

  // Get top node (<map>)
  auto* mapNode = worldXml.first_node();

  // Read width and height
  auto* widthAttr = mapNode->first_attribute("width");
  auto* heightAttr = mapNode->first_attribute("height");
  if (widthAttr == nullptr || heightAttr == nullptr)
  {
    LOG_ERROR("%s: Invalid file, missing attributes width or height in <map>-node", __func__);
    free(xmlString);
    return std::unique_ptr<World>();
  }

  int worldSizeX = std::stoi(widthAttr->value());
  int worldSizeY = std::stoi(heightAttr->value());

  // Read tiles
  std::unordered_map<Position, Tile, Position::Hash> tiles;
  auto* tileNode = mapNode->first_node();
  for (int y = worldSizeStart_; y < worldSizeStart_ + worldSizeY; y++)
  {
    for (int x = worldSizeStart_; x < worldSizeStart_ + worldSizeX; x++)
    {
      Position position(x, y, 7);

      if (tileNode == nullptr)
      {
        LOG_ERROR("%s: Invalid file, missing <tile>-node", __func__);
        free(xmlString);
        return std::unique_ptr<World>();
      }

      // Read the first <item> (there must be at least one, the ground item)
      // TODO(gurka): Must there be one? What about "void", or is it also an Item?
      auto* groundItemNode = tileNode->first_node();
      if (groundItemNode == nullptr)
      {
        LOG_ERROR("%s: Invalid file, <tile>-node is missing <item>-node", __func__);
        free(xmlString);
        return std::unique_ptr<World>();
      }
      auto* groundItemAttr = groundItemNode->first_attribute("id");
      if (groundItemAttr == nullptr)
      {
        LOG_ERROR("%s: Invalid file, missing attribute id in <item>-node", __func__);
        free(xmlString);
        return std::unique_ptr<World>();
      }

      auto groundItemId = std::stoi(groundItemAttr->value());
      auto groundItem = itemFactory->createItem(groundItemId);
      tiles.emplace(std::piecewise_construct,
                    std::forward_as_tuple(position),
                    std::forward_as_tuple(groundItem));

      // Read more items to put in this tile
      // But due to the way otserv-3.0 made world.xml, do it backwards
      for (auto* itemNode = tileNode->last_node(); itemNode != groundItemNode; itemNode = itemNode->previous_sibling())
      {
        auto* itemIdAttr = itemNode->first_attribute("id");
        if (itemIdAttr == nullptr)
        {
          LOG_DEBUG("%s: Missing attribute id in <item>-node, skipping Item", __func__);
          continue;
        }

        auto itemId = std::stoi(itemIdAttr->value());
        tiles.at(position).addItem(itemFactory->createItem(itemId));
      }

      // Go to next <tile> in XML
      tileNode = tileNode->next_sibling();
    }
  }

  LOG_INFO("World loaded, size: %d x %d", worldSizeX, worldSizeY);
  free(xmlString);

  return std::unique_ptr<World>(new World(std::move(itemFactory), worldSizeX, worldSizeY, std::move(tiles)));
}
