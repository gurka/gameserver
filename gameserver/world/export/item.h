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

#ifndef WORLD_EXPORT_ITEM_H_
#define WORLD_EXPORT_ITEM_H_

#include <cstdint>
#include <string>

using ItemUniqueId = std::uint64_t;
using ItemTypeId = int;

struct ItemType;

class Item
{
 public:
  static constexpr ItemUniqueId INVALID_UNIQUE_ID = 0u;

  virtual ~Item() = default;

  virtual ItemUniqueId getItemUniqueId() const = 0;
  virtual ItemTypeId getItemTypeId() const = 0;

  virtual const ItemType& getItemType() const = 0;

  virtual int getCount() const = 0;
  virtual void setCount(int count) = 0;

  bool operator==(const Item& other) const
  {
    return getItemUniqueId() == other.getItemUniqueId();
  }

  bool operator!=(const Item& other) const
  {
    return !(*this == other);
  }
};

struct ItemType
{
  ItemTypeId id     = 0;

  // Loaded from data file
  bool ground       = false;
  int  speed        = 0;
  bool isBlocking   = false;
  bool alwaysOnTop  = false;
  bool isContainer  = false;
  bool isStackable  = false;
  bool isUsable     = false;
  bool isMultitype  = false;
  bool isNotMovable = false;
  bool isEquipable  = false;

  // Loaded from xml file
  std::string name     = "";
  int weight           = 0;
  int decayto          = 0;
  int decaytime        = 0;
  int damage           = 0;
  int maxitems         = 0;
  std::string type     = "";
  std::string position = "";
  int attack           = 0;
  int defence          = 0;
  int arm              = 0;
  std::string skill    = "";
  std::string descr    = "";
  int handed           = 0;
  int shottype         = 0;
  std::string amutype  = "";
};

#endif  // WORLD_EXPORT_ITEM_H_
