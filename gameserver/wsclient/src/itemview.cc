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
#include <cstdint>
#include <string>
#include <vector>

#include <emscripten.h>

#include "logger.h"
#include "data_loader.h"
#include "sprite_loader.h"

int main()
{
  constexpr auto data_filename = "files/data.dat";
  constexpr auto sprite_filename = "files/sprite.dat";

  io::data_loader::ItemTypes item_types;
  common::ItemTypeId item_type_id_first;
  common::ItemTypeId item_type_id_last;
  if (!io::data_loader::load(data_filename, &item_types, &item_type_id_first, &item_type_id_last))
  {
    LOG_ERROR("Could not load data");
    return 1;
  }

  io::SpriteLoader sprite_loader;
  if (!sprite_loader.load(sprite_filename))
  {
    LOG_ERROR("Could not load sprites");
    return 1;
  }

  LOG_INFO("itemview started");
}

