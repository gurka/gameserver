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

#include "world_factory.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

#include "logger.h"
#include "world_loader.h"
#include "item_manager.h"
#include "item.h"
#include "tile.h"
#include "world.h"

namespace gameengine
{

// TODO(simon): is this file/class/function even necessary now?
std::unique_ptr<world::World> WorldFactory::createWorld(const std::string& world_filename,
                                                        ItemManager* item_manager)
{
  const auto create_item = [&item_manager](common::ItemTypeId item_type_id)
  {
    return item_manager->createItem(item_type_id);
  };

  const auto get_item = [&item_manager](common::ItemUniqueId item_unique_id)
  {
    return item_manager->getItem(item_unique_id);
  };

  auto world_data = world_loader::load(world_filename, create_item, get_item);
  if (world_data.tiles.empty())
  {
    // io::world_loader::load should log error
    return {};
  }

  LOG_INFO("World loaded, size: %d x %d", world_data.world_size_x, world_data.world_size_y);

  return std::make_unique<world::World>(world_data.world_size_x,  // NOLINT bug in clang-tidy used in Travis (?)
                                        world_data.world_size_y,
                                        std::move(world_data.tiles));
}

}  // namespace gameengine
