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

#include "player.h"

#include <utility>

#include "logger.h"

bool Equipment::canAddItem(const Item& item, int inventorySlot) const
{
  if (inventorySlot < 1 || inventorySlot > 10)
  {
    LOG_ERROR("%s: inventorySlot: %d is invalid", __func__, inventorySlot);
    return false;
  }

  // TODO(simon): Check capacity

  // Get Item attributes
  std::string itemType;
  std::string itemPosition;

  if (!item.getItemType().type.empty())
  {
    itemType = item.getItemType().type;
  }

  if (!item.getItemType().position.empty())
  {
    itemPosition = item.getItemType().position;
  }

  LOG_DEBUG("canAddItem(): ItemTypeId: %d Type: %s Positon: %s",
            item.getItemTypeId(),
            itemType.c_str(),
            itemPosition.c_str());

  switch (inventorySlot)
  {
    case HELMET:
    {
      return itemType == "armor" && itemPosition == "helmet";
    }

    case AMULET:
    {
      return itemType == "armor" && itemPosition == "amulet";
    }

    case BACKPACK:
    {
      return itemType == "container";
    }

    case ARMOR:
    {
      return itemType == "armor" && itemPosition == "body";
    }

    case RIGHT_HAND:
    case LEFT_HAND:
    {
      // Just check that we don't equip an 2-hander if other hand is not empty
      if (item.getItemType().handed == 2)
      {
        if (inventorySlot == RIGHT_HAND)
        {
          return items_.at(LEFT_HAND) == nullptr;
        }
        else
        {
          return items_.at(RIGHT_HAND) == nullptr;
        }
      }
      return true;
    }

    case LEGS:
    {
      return itemType == "armor" && itemPosition == "legs";
    }

    case FEET:
    {
      return itemType == "armor" && itemPosition == "boots";
    }

    case RING:
    {
      return itemType == "armor" && itemPosition == "ring";
    }

    case AMMO:
    {
      // TODO(simon): Not yet in items.xml
      return itemType == "ammo";
    }
  }

  return false;
}

bool Equipment::addItem(const Item& item, int inventorySlot)
{
  if (inventorySlot < 1 || inventorySlot > 10)
  {
    LOG_ERROR("%s: inventorySlot: %d is invalid", __func__, inventorySlot);
    return false;
  }

  if (!canAddItem(item, inventorySlot))
  {
    return false;
  }

  items_[inventorySlot] = &item;
  return true;
}

bool Equipment::removeItem(ItemTypeId itemTypeId, int inventorySlot)
{
  if (inventorySlot < 1 || inventorySlot > 10)
  {
    LOG_ERROR("%s: inventorySlot: %d is invalid", __func__, inventorySlot);
    return false;
  }

  if (items_[inventorySlot]->getItemTypeId() != itemTypeId)
  {
    return false;
  }

  items_[inventorySlot] = nullptr;
  return true;
}

const Item* Equipment::getItem(int inventorySlot) const
{
  if (inventorySlot < 1 || inventorySlot > 10)
  {
    LOG_ERROR("%s: inventorySlot: %d is invalid", __func__, inventorySlot);
    return nullptr;
  }

  return items_[inventorySlot];
}

Player::Player(const std::string& name)
  : Creature(name),
    maxMana_(100),
    mana_(100),
    capacity_(300),
    experience_(4200),
    magicLevel_(1),
    partyShield_(0)
{
}

std::uint16_t Player::getSpeed() const
{
  return 220 + (2 * (getLevel() - 1));
}

std::uint8_t Player::getLevel() const
{
  if (experience_ < 100) return 1;
  else if (experience_ < 200) return 2;
  else if (experience_ < 400) return 3;
  else if (experience_ < 800) return 4;
  else if (experience_ < 1500) return 5;
  else if (experience_ < 2600) return 6;
  else if (experience_ < 4200) return 7;
  else return 8;
}
