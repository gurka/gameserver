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
#include "data_loader.h"

namespace gameengine
{

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

  world::ItemUniqueId createItem(world::ItemTypeId item_type_id);
  void destroyItem(world::ItemUniqueId item_unique_id);

  world::Item* getItem(world::ItemUniqueId item_unique_id);

 private:
  bool loadItemTypesItemsFile(const std::string& items_filename);
  void dumpItemTypeToJson() const;

  class ItemImpl : public world::Item
  {
   public:
    ItemImpl(world::ItemUniqueId item_unique_id, const world::ItemType* item_type)
      : m_item_unique_id(item_unique_id),
        m_itemType(item_type),
        m_count(1)
    {
    }

    world::ItemUniqueId getItemUniqueId() const override { return m_item_unique_id; }
    world::ItemTypeId getItemTypeId() const override { return m_itemType->id; }

    const world::ItemType& getItemType() const override { return *m_itemType; }

    std::uint8_t getCount() const override { return m_count; }
    void setCount(std::uint8_t count) override { m_count = count; }

   private:
    world::ItemUniqueId m_item_unique_id;
    const world::ItemType* m_itemType;
    std::uint8_t m_count;
  };

  std::unordered_map<world::ItemUniqueId, ItemImpl> m_items;
  world::ItemUniqueId m_next_item_unique_id{1};

  io::data_loader::ItemTypes  m_item_types;
  world::ItemTypeId m_item_types_id_first{0};
  world::ItemTypeId m_item_types_id_last{0};
};

}  // namespace gameengine

#endif  // GAMEENGINE_SRC_ITEM_MANAGER_H_
