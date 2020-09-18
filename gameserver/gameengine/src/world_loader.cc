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

#include "world_loader.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

#include "logger.h"
#include "item.h"
#include "position.h"
#include "tile.h"
#include "world.h"

#include "rapidxml.hpp"

namespace gameengine::world_loader
{

WorldData load(const std::string& filename,
               const CreateItem& create_item,
               const GetItem& get_item)
{
  WorldData world_data;

  // Open world.xml and read it into a string
  LOG_INFO("Loading world file: \"%s\"", filename.c_str());
  std::ifstream xml_file(filename);
  if (!xml_file.is_open())
  {
    LOG_ERROR("%s: Could not open file: \"%s\"", __func__, filename.c_str());
    return world_data;
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
  rapidxml::xml_document<> world_xml;
  world_xml.parse<0>(xml_string);

  // Get top node (<map>)
  const auto* map_node = world_xml.first_node();

  // Read width and height
  const auto* width_attr = map_node->first_attribute("width");
  const auto* height_attr = map_node->first_attribute("height");
  if (width_attr == nullptr || height_attr == nullptr)
  {
    LOG_ERROR("%s: Invalid file, missing attributes width or height in <map>-node", __func__);
    free(xml_string);
    return world_data;
  }

  world_data.world_size_x = std::stoi(width_attr->value());
  world_data.world_size_y = std::stoi(height_attr->value());

  // Read tiles
  std::vector<world::Tile> tiles;
  tiles.reserve(world_data.world_size_x * world_data.world_size_y);
  const auto* tile_node = map_node->first_node();
  for (int y = world::POSITION_OFFSET; y < world::POSITION_OFFSET + world_data.world_size_y; y++)
  {
    for (int x = world::POSITION_OFFSET; x < world::POSITION_OFFSET + world_data.world_size_x; x++)
    {
      if (tile_node == nullptr)
      {
        LOG_ERROR("%s: Invalid file, missing <tile>-node", __func__);
        free(xml_string);
        return world_data;
      }

      // Read the first <item> (there must be at least one, the ground item)
      // TODO(simon): Must there be one? What about "void", or is it also an Item?
      const auto* ground_item_node = tile_node->first_node();
      if (ground_item_node == nullptr)
      {
        LOG_ERROR("%s: Invalid file, <tile>-node is missing <item>-node", __func__);
        free(xml_string);
        return world_data;
      }
      const auto* ground_item_attr = ground_item_node->first_attribute("id");
      if (ground_item_attr == nullptr)
      {
        LOG_ERROR("%s: Invalid file, missing attribute id in <item>-node", __func__);
        free(xml_string);
        return world_data;
      }

      const auto ground_item_type_id = std::stoi(ground_item_attr->value());
      const auto ground_item_id = create_item(ground_item_type_id);
      if (ground_item_id == common::Item::INVALID_UNIQUE_ID)
      {
        LOG_ERROR("%s: ground_item_type_id: %d is invalid", __func__, ground_item_type_id);
        free(xml_string);
        return world_data;
      }

      tiles.emplace_back(get_item(ground_item_id));

      // Read more items to put in this tile
      // But due to the way otserv-3.0 made world.xml, do it backwards
      for (auto* item_node = tile_node->last_node();
           item_node != ground_item_node;
           item_node = item_node->previous_sibling())
      {
        const auto* item_id_attr = item_node->first_attribute("id");
        if (item_id_attr == nullptr)
        {
          LOG_DEBUG("%s: Missing attribute id in <item>-node, skipping Item", __func__);
          continue;
        }

        const auto item_type_id = std::stoi(item_id_attr->value());
        const auto item_id = create_item(item_type_id);
        if (item_id == common::Item::INVALID_UNIQUE_ID)
        {
          LOG_ERROR("%s: item_type_id: %d is invalid", __func__, item_type_id);
          free(xml_string);
          return world_data;
        }

        tiles.back().addThing(get_item(item_id));
      }

      // Go to next <tile> in XML
      tile_node = tile_node->next_sibling();
    }
  }
  free(xml_string);

  // Transpose the tiles
  // TODO(simon): Fix this, or rather, fix the input world xml
  world_data.tiles.reserve(world_data.world_size_x * world_data.world_size_y);
  for (int y = 0; y < world_data.world_size_y; y++)
  {
    for (int x = 0; x < world_data.world_size_x; x++)
    {
      const auto index = x * world_data.world_size_y + y;
      world_data.tiles.push_back(std::move(tiles[index]));
    }
  }

  return world_data;
}

}  // namespace gameengine::world_loader

