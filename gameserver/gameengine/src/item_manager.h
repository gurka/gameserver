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

#ifndef GAMEENGINE_SRC_ITEM_MANAGER_H_
#define GAMEENGINE_SRC_ITEM_MANAGER_H_

#include <cstdint>
#include <array>
#include <unordered_map>

namespace new_item
{

using ItemId = std::uint64_t;
using ItemTypeId = int;

struct ItemType;

class Item
{
 public:
  virtual ~Item() = default;

  virtual ItemId getItemId() const = 0;
  virtual const ItemType& getItemType() const = 0;
};

struct ItemType
{
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

class ItemManager
{
 public:
  ItemManager()
    : items_(),
      nextItemId_(0),
      itemTypes_(),
      itemTypesIdFirst_(0),
      itemTypesIdLast_(0)
  {
  }

  bool loadItemTypes(const std::string& dataFilename, const std::string& itemsFilename);

  ItemId createItem(ItemTypeId itemTypeId);
  void destroyItem(ItemId itemId);

  Item* getItem(ItemId itemId);

 private:
  bool loadItemTypesDataFile(const std::string& dataFilename);
  bool loadItemTypesItemsFile(const std::string& itemsFilename);

  class ItemImpl : public Item
  {
   public:
    ItemImpl(ItemId itemId, const ItemType* itemType)
      : itemId_(itemId),
        itemType_(itemType)
    {
    }

    ItemId getItemId() const override { return itemId_; }
    const ItemType& getItemType() const override { return *itemType_; }

   private:
    ItemId itemId_;
    const ItemType* itemType_;
  };

  std::unordered_map<ItemId, ItemImpl> items_;
  ItemId nextItemId_;

  std::array<ItemType, 4096> itemTypes_;
  ItemTypeId itemTypesIdFirst_;
  ItemTypeId itemTypesIdLast_;
};

}

#endif  // GAMEENGINE_SRC_ITEM_MANAGER_H_
