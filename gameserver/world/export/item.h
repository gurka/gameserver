/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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

#include <string>
#include <unordered_map>

using ItemId = int;

struct ItemData
{
  static const ItemId INVALID_ID;

  // Loaded from data file
  ItemId id         = INVALID_ID;
  bool ground       = false;
  int speed         = 0;
  bool isBlocking   = false;
  bool alwaysOnTop  = false;
  bool isContainer  = false;
  bool isStackable  = false;
  bool isUsable     = false;
  bool isMultitype  = false;
  bool isNotMovable = false;
  bool isEquipable  = false;

  // Loaded from items.xml
  std::string name  = "";
  std::unordered_map<std::string, std::string> attributes;
};

class Item
{
 public:
  Item()
    : itemData_(nullptr),
      count_(0)
  {
  }

  explicit Item(const ItemData* itemData)
    : itemData_(itemData),
      count_((itemData_ != nullptr) ? 1 : 0)
  {
  }

  virtual ~Item() = default;

  static bool loadItemData(const std::string& dataFilename, const std::string& itemsFilename);

  bool isValid() const { return itemData_ != nullptr; }

  // Loaded from data file
  ItemId getItemId() const { return itemData_->id; }
  bool isGround() const { return itemData_->ground; }
  int getSpeed() const { return itemData_->speed; }
  bool isBlocking() const { return itemData_->isBlocking; }
  bool alwaysOnTop() const { return itemData_->alwaysOnTop; }
  bool isContainer() const { return itemData_->isContainer; }
  bool isStackable() const { return itemData_->isStackable; }
  bool isUsable() const { return itemData_->isUsable; }
  bool isMultitype() const { return itemData_->isMultitype; }
  bool isNotMovable() const { return itemData_->isNotMovable; }
  bool isEquipable() const { return itemData_->isEquipable; }

  // Loaded from items.xml
  const std::string& getName() const { return itemData_->name; }

  bool hasAttribute(const std::string& name) const { return itemData_->attributes.count(name) == 1; }

  template<typename T>
  T getAttribute(const std::string& name) const;

  // For this specific Item
  void setCount(int count) { count_ = count; }
  int getCount() const { return count_; }

  int getSubtype() const { return 0; }  // TODO(gurka): ??

  bool operator==(const Item& other) const;
  bool operator!=(const Item& other) const;

 private:
  const ItemData* itemData_;
  uint8_t count_;
};

#endif  // WORLD_ITEM_H_

