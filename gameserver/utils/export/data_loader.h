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

#ifndef UTILS_EXPORT_DATA_LOADER_H_
#define UTILS_EXPORT_DATA_LOADER_H_

#include <array>
#include <string>

#include "item.h"

namespace utils::data_loader
{

constexpr auto MAX_ITEM_TYPES = 4096;
using ItemTypes = std::array<common::ItemType, MAX_ITEM_TYPES>;
bool load(const std::string& data_filename,
          ItemTypes* item_types,
          common::ItemTypeId* id_first,
          common::ItemTypeId* id_last);
bool loadXml(const std::string& items_filename,
             ItemTypes* item_types,
             common::ItemTypeId id_first,
             common::ItemTypeId id_last);
void dumpToJson(const ItemTypes& item_types,
                common::ItemTypeId id_first,
                common::ItemTypeId id_last);

}  // namespace utils::data_loader

#endif  // UTILS_EXPORT_DATA_LOADER_H_
