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
#include <string>
#include <unordered_map>

#include "item.h"

class ItemManager
{
 public:
  ItemManager()
    : items_(),
      nextItemUniqueId_(1),
      itemTypes_(),
      itemTypesIdFirst_(0),
      itemTypesIdLast_(0)
  {
  }

  bool loadItemTypes(const std::string& dataFilename, const std::string& itemsFilename);

  ItemUniqueId createItem(ItemTypeId itemTypeId);
  void destroyItem(ItemUniqueId itemUniqueId);

  Item* getItem(ItemUniqueId itemUniqueId);

 private:
  bool loadItemTypesDataFile(const std::string& dataFilename);
  bool loadItemTypesItemsFile(const std::string& itemsFilename);
  void dumpItemTypeToJson() const;

  class ItemImpl : public Item
  {
   public:
    ItemImpl(ItemUniqueId itemUniqueId, const ItemType* itemType)
      : itemUniqueId_(itemUniqueId),
        itemType_(itemType),
        count_(1)
    {
    }

    ItemUniqueId getItemUniqueId() const override { return itemUniqueId_; }
    ItemTypeId getItemTypeId() const override { return itemType_->id; }

    const ItemType& getItemType() const override { return *itemType_; }

    std::uint8_t getCount() const override { return count_; }
    void setCount(std::uint8_t count) override { count_ = count; }

   private:
    ItemUniqueId itemUniqueId_;
    const ItemType* itemType_;
    std::uint8_t count_;
  };

  std::unordered_map<ItemUniqueId, ItemImpl> items_;
  ItemUniqueId nextItemUniqueId_;

  std::array<ItemType, 4096> itemTypes_;
  ItemTypeId itemTypesIdFirst_;
  ItemTypeId itemTypesIdLast_;
};

#endif  // GAMEENGINE_SRC_ITEM_MANAGER_H_
