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

#ifndef WORLD_ITEM_H_
#define WORLD_ITEM_H_

#include <cstdint>
#include <array>
#include <string>
#include <unordered_map>

using ItemId = int;

struct ItemData
{
  bool valid        = false;

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

  // Loaded from item file
  std::string name = "";

  // TODO(gurka): Change to std::vector ?
  std::unordered_map<std::string, std::string> attributes;
};

class Item
{
 public:
  // Loads ItemData from the data file and the item file
  // Must be loaded (successfully) before any Item objects are created
  static bool loadItemData(const std::string& dataFilename, const std::string& itemsFilename);

  Item()
    : id_(INVALID_ID),
      count_(0),
      itemData_(&itemDatas_[id_]),  // Will point to an invalid ItemData
      containerId_(INVALID_ID)
  {
  }

  explicit Item(ItemId itemId)
    : id_(itemId),
      count_(1),
      itemData_(&itemDatas_[id_]),
      containerId_(INVALID_ID)
  {
  }

  Item(ItemId itemId, int containerId)
      : id_(itemId),
        count_(1),
        itemData_(&itemDatas_[id_]),
        containerId_(containerId)
  {
  }

  bool isValid() const { return itemData_->valid; }

  // Specific for this distinct Item
  ItemId getItemId() const { return id_; }
  uint8_t getCount() const { return count_; }

  // Loaded from data file
  bool isGround()     const { return itemData_->ground; }
  int getSpeed()      const { return itemData_->speed; }
  bool isBlocking()   const { return itemData_->isBlocking; }
  bool alwaysOnTop()  const { return itemData_->alwaysOnTop; }
  bool isContainer()  const { return itemData_->isContainer; }
  bool isStackable()  const { return itemData_->isStackable; }
  bool isUsable()     const { return itemData_->isUsable; }
  bool isMultitype()  const { return itemData_->isMultitype; }
  bool isNotMovable() const { return itemData_->isNotMovable; }
  bool isEquipable()  const { return itemData_->isEquipable; }
  int getSubtype()    const { return 0; }  // TODO(gurka): ??

  // Loaded from items.xml
  const std::string& getName() const { return itemData_->name; }

  bool hasAttribute(const std::string& name) const { return itemData_->attributes.count(name) == 1; }

  template<typename T>
  T getAttribute(const std::string& name) const;

  int getContainerId() const { return containerId_; }

#ifdef UNITTEST
  static void setItemData(ItemId itemId, const ItemData& itemData) { itemDatas_[itemId] = itemData; }
#endif

 private:
  static constexpr ItemId INVALID_ID = 0;

  static constexpr std::size_t MAX_ITEM_DATAS = 3072;
  static std::array<ItemData, MAX_ITEM_DATAS> itemDatas_;

  ItemId id_;
  uint8_t count_;
  ItemData* itemData_;

  // TODO(gurka): Try to refactor everything below as only certain types of items uses these values
  int containerId_;
};

#endif  // WORLD_ITEM_H_
