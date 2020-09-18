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

#ifndef GAMEENGINE_SRC_WORLD_LOADER_H_
#define GAMEENGINE_SRC_WORLD_LOADER_H_

#include <functional>
#include <memory>
#include <string>

#include "tile.h"
#include "item.h"

namespace gameengine::world_loader
{

using CreateItem = std::function<common::ItemUniqueId(common::ItemTypeId)>;
using GetItem = std::function<const common::Item*(common::ItemUniqueId)>;

struct WorldData
{
  int world_size_x;
  int world_size_y;
  std::vector<world::Tile> tiles;
};

WorldData load(const std::string& filename,
               const CreateItem& create_item,
               const GetItem& get_item);

}  // namespace gameengine::world_loader

#endif  // GAMEENGINE_SRC_WORLD_LOADER_H_
