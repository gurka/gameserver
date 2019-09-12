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
    : m_next_item_unique_id(1),
      m_item_types_id_first(0),
      m_item_types_id_last(0)
  {
  }

  bool loadItemTypes(const std::string& data_filename, const std::string& items_filename);

  ItemUniqueId createItem(ItemTypeId item_type_id);
  void destroyItem(ItemUniqueId item_unique_id);

  Item* getItem(ItemUniqueId item_unique_id);

 private:
  bool loadItemTypesDataFile(const std::string& data_filename);
  bool loadItemTypesItemsFile(const std::string& items_filename);
  void dumpItemTypeToJson() const;

  class ItemImpl : public Item
  {
   public:
    ItemImpl(ItemUniqueId item_unique_id, const ItemType* item_type)
      : m_item_unique_id(item_unique_id),
        m_itemType(item_type),
        m_count(1)
    {
    }

    ItemUniqueId getItemUniqueId() const override { return m_item_unique_id; }
    ItemTypeId getItemTypeId() const override { return m_itemType->id; }

    const ItemType& getItemType() const override { return *m_itemType; }

    std::uint8_t getCount() const override { return m_count; }
    void setCount(std::uint8_t count) override { m_count = count; }

   private:
    ItemUniqueId m_item_unique_id;
    const ItemType* m_itemType;
    std::uint8_t m_count;
  };

  std::unordered_map<ItemUniqueId, ItemImpl> m_items;
  ItemUniqueId m_next_item_unique_id{1};

  std::array<ItemType, 4096> m_item_types;
  ItemTypeId m_item_types_id_first{0};
  ItemTypeId m_item_types_id_last{0};
};

#endif  // GAMEENGINE_SRC_ITEM_MANAGER_H_
