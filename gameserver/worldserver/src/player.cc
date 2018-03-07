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

Equipment::Equipment()
{
  items_.insert(std::make_pair(Slot::HELMET,     Item()));
  items_.insert(std::make_pair(Slot::AMULET,     Item()));
  items_.insert(std::make_pair(Slot::BACKPACK,   Item(1411, 1)));  // Backpack, containerId = 1
  items_.insert(std::make_pair(Slot::ARMOR,      Item()));
  items_.insert(std::make_pair(Slot::RIGHT_HAND, Item()));
  items_.insert(std::make_pair(Slot::LEFT_HAND,  Item()));
  items_.insert(std::make_pair(Slot::LEGS,       Item()));
  items_.insert(std::make_pair(Slot::FEET,       Item()));
  items_.insert(std::make_pair(Slot::RING,       Item()));
  items_.insert(std::make_pair(Slot::AMMO,       Item()));
}

bool Equipment::canAddItem(const Item& item, uint8_t inventoryIndex) const
{
  Slot slot = static_cast<Slot>(inventoryIndex);

  // TODO(gurka): Check capacity

  // First check if the slot is empty
  if (items_.at(slot).isValid())
  {
    return false;
  }

  // Get Item attributes
  std::string itemType;
  std::string itemPosition;

  if (item.hasAttribute("type"))
  {
    itemType = item.getAttribute<std::string>("type");
  }

  if (item.hasAttribute("position"))
  {
    itemPosition = item.getAttribute<std::string>("position");
  }

  LOG_DEBUG("canAddItem(): Item: %d Type: %s Positon: %s", item.getItemId(), itemType.c_str(), itemPosition.c_str());

  switch (slot)
  {
    case Slot::HELMET:
    {
      return itemType == "armor" && itemPosition == "helmet";
    }

    case Slot::AMULET:
    {
      return itemType == "armor" && itemPosition == "amulet";
    }

    case Slot::BACKPACK:
    {
      return itemType == "container";
    }

    case Slot::ARMOR:
    {
      return itemType == "armor" && itemPosition == "body";
    }

    case Slot::RIGHT_HAND:
    case Slot::LEFT_HAND:
    {
      // Just check that we don't equip an 2-hander if other hand is not empty
      if (item.hasAttribute("handed"))
      {
        // Actually we could stop here, since only 2-handers have this attribute
        // But anyway ...
        if (item.getAttribute<int>("handed") == 2)
        {
          if (slot == Slot::RIGHT_HAND)
          {
            return !items_.at(Slot::LEFT_HAND).isValid();
          }
          else
          {
            return !items_.at(Slot::RIGHT_HAND).isValid();
          }
        }
      }
      return true;
    }

    case Slot::LEGS:
    {
      return itemType == "armor" && itemPosition == "legs";
    }

    case Slot::FEET:
    {
      return itemType == "armor" && itemPosition == "boots";
    }

    case Slot::RING:
    {
      return itemType == "armor" && itemPosition == "ring";
    }

    case Slot::AMMO:
    {
      // TODO(gurka): Not yet in items.xml
      return itemType == "ammo";
    }
  }

  return false;
}

bool Equipment::addItem(const Item& item, uint8_t inventoryIndex)
{
  if (canAddItem(item, inventoryIndex))
  {
    items_.at(static_cast<Slot>(inventoryIndex)) = item;
    return true;
  }
  else
  {
    return false;
  }
}

bool Equipment::removeItem(ItemId itemId, uint8_t inventoryIndex)
{
  Slot slot = static_cast<Slot>(inventoryIndex);
  if (items_.at(slot).getItemId() == itemId)
  {
    items_.at(slot) = Item();
    return true;
  }
  else
  {
    return false;
  }
}

Player::Player(const std::string& name)
  : Creature(name),
    maxMana_(100),
    mana_(100),
    capacity_(300),
    experience_(4200),
    magicLevel_(1),
    partyShield_(0),
    queuedMoves_()
{
}

int Player::getSpeed() const
{
  return 220 + (2 * (getLevel() - 1));
}

int Player::getLevel() const
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
