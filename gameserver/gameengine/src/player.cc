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

#include "player.h"

#include <utility>

#include "logger.h"

namespace gameengine
{

bool Equipment::canAddItem(const common::Item& item, int inventory_slot) const
{
  if (inventory_slot < 1 || inventory_slot > 10)
  {
    LOG_ERROR("%s: inventory_slot: %d is invalid", __func__, inventory_slot);
    return false;
  }

  // TODO(simon): Check capacity

  // Get Item attributes
  std::string item_type;
  std::string item_position;

  if (!item.getItemType().type_xml.empty())
  {
    item_type = item.getItemType().type_xml;
  }

  if (!item.getItemType().position.empty())
  {
    item_position = item.getItemType().position;
  }

  LOG_DEBUG("canAddItem(): ItemTypeId: %d Type: %s Positon: %s",
            item.getItemTypeId(),
            item_type.c_str(),
            item_position.c_str());

  switch (inventory_slot)
  {
    case HELMET:
    {
      return item_type == "armor" && item_position == "helmet";
    }

    case AMULET:
    {
      return item_type == "armor" && item_position == "amulet";
    }

    case BACKPACK:
    {
      return item_type == "container";
    }

    case ARMOR:
    {
      return item_type == "armor" && item_position == "body";
    }

    case RIGHT_HAND:
    case LEFT_HAND:
    {
      // Just check that we don't equip an 2-hander if other hand is not empty
      if (item.getItemType().handed == 2)
      {
        if (inventory_slot == RIGHT_HAND)
        {
          return m_items.at(LEFT_HAND) == nullptr;
        }

        return m_items.at(RIGHT_HAND) == nullptr;
      }
      return true;
    }

    case LEGS:
    {
      return item_type == "armor" && item_position == "legs";
    }

    case FEET:
    {
      return item_type == "armor" && item_position == "boots";
    }

    case RING:
    {
      return item_type == "armor" && item_position == "ring";
    }

    case AMMO:
    {
      // TODO(simon): Not yet in items.xml
      return item_type == "ammo";
    }
  }

  return false;
}

bool Equipment::addItem(const common::Item& item, int inventory_slot)
{
  if (inventory_slot < 1 || inventory_slot > 10)
  {
    LOG_ERROR("%s: inventory_slot: %d is invalid", __func__, inventory_slot);
    return false;
  }

  if (!canAddItem(item, inventory_slot))
  {
    return false;
  }

  m_items[inventory_slot] = &item;
  return true;
}

bool Equipment::removeItem(common::ItemTypeId item_type_id, int inventory_slot)
{
  if (inventory_slot < 1 || inventory_slot > 10)
  {
    LOG_ERROR("%s: inventory_slot: %d is invalid", __func__, inventory_slot);
    return false;
  }

  if (m_items[inventory_slot]->getItemTypeId() != item_type_id)
  {
    return false;
  }

  m_items[inventory_slot] = nullptr;
  return true;
}

const common::Item* Equipment::getItem(int inventory_slot) const
{
  if (inventory_slot < 1 || inventory_slot > 10)
  {
    LOG_ERROR("%s: inventory_slot: %d is invalid", __func__, inventory_slot);
    return nullptr;
  }

  return m_items[inventory_slot];
}

Player::Player(common::CreatureId creature_id, const std::string& name)
  : Creature(creature_id, name),
    m_max_mana(100),
    m_mana(100),
    m_capacity(300),
    m_experience(4200),
    m_magic_level(1),
    m_party_shield(0)
{
}

std::uint16_t Player::getSpeed() const
{
  return 220 + (2 * (getLevel() - 1));
}

std::uint8_t Player::getLevel() const
{
  if (m_experience <  100) { return 1; }
  if (m_experience <  200) { return 2; }
  if (m_experience <  400) { return 3; }
  if (m_experience <  800) { return 4; }
  if (m_experience < 1500) { return 5; }
  if (m_experience < 2600) { return 6; }
  if (m_experience < 4200) { return 7; }
  return 8;
}

}  // namespace gameengine
