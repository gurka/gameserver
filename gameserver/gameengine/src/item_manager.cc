/* The MIT License (MIT)
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

#include "item_manager.h"

#include "logger.h"

namespace gameengine
{

bool ItemManager::loadItemTypes(const std::string& data_filename, const std::string& items_filename)
{
  if (!utils::data_loader::load(data_filename, &m_item_types, &m_item_types_id_first, &m_item_types_id_last))
  {
    LOG_ERROR("%s: could not load datafile: %s", __func__, data_filename.c_str());
    return false;
  }


  if (!utils::data_loader::loadXml(items_filename, &m_item_types, m_item_types_id_first, m_item_types_id_last))
  {
    LOG_ERROR("%s: could not load itemsfile: %s", __func__, items_filename.c_str());
    return false;
  }

  return true;
}

common::ItemUniqueId ItemManager::createItem(common::ItemTypeId item_type_id)
{
  if (item_type_id < m_item_types_id_first || item_type_id > m_item_types_id_last)
  {
    LOG_ERROR("%s: item_type_id: %d out of range", __func__, item_type_id);
    return common::Item::INVALID_UNIQUE_ID;
  }

  const auto item_unique_id = m_next_item_unique_id;
  ++m_next_item_unique_id;

  m_items.emplace(item_unique_id, ItemImpl(item_unique_id, &m_item_types[item_type_id]));
  LOG_DEBUG("%s: created Item with item_unique_id: %lu, item_type_id: %d", __func__, item_unique_id, item_type_id);

  return item_unique_id;
}

void ItemManager::destroyItem(common::ItemUniqueId item_unique_id)
{
  if (m_items.count(item_unique_id) == 0)
  {
    LOG_ERROR("%s: could not find Item with item_unique_id: %lu", __func__, item_unique_id);
    return;
  }

  LOG_DEBUG("%s: destroying Item with item_unique_id: %lu", __func__, item_unique_id);
  m_items.erase(item_unique_id);
}

common::Item* ItemManager::getItem(common::ItemUniqueId item_unique_id)
{
  if (m_items.count(item_unique_id) == 0)
  {
    return nullptr;
  }

  return &m_items.at(item_unique_id);
}

}  // namespace gameengine
