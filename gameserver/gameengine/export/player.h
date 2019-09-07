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

#ifndef GAMEENGINE_SRC_PLAYER_H_
#define GAMEENGINE_SRC_PLAYER_H_

#include <array>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>

#include "creature.h"
#include "direction.h"
#include "item.h"

class Equipment
{
 public:
  Equipment()
    : items_()
  {
  }

  const Item* getItem(int inventorySlot) const;
  Item* getItem(int inventorySlot);

  bool canAddItem(const Item& item, int inventorySlot) const;
  bool addItem(Item* item, int inventorySlot);
  bool removeItem(ItemTypeId itemTypeId, int inventorySlot);

 private:
  std::array<Item*, 11> items_;  // index 0 is invalid
  enum InventorySlotInfo
  {
    HELMET     = 1,
    AMULET     = 2,
    BACKPACK   = 3,
    ARMOR      = 4,
    RIGHT_HAND = 5,
    LEFT_HAND  = 6,
    LEGS       = 7,
    FEET       = 8,
    RING       = 9,
    AMMO       = 10,
  };
};

class Player : public Creature
{
 public:
  explicit Player(const std::string& name);

  // From Creature
  std::uint16_t getSpeed() const override;

  std::uint16_t getMaxMana() const { return maxMana_; }
  void setMaxMana(std::uint16_t maxMana) { maxMana_ = maxMana; }

  std::uint16_t getMana() const { return mana_; }
  void setMana(std::uint16_t mana) { mana_ = mana; }

  std::uint16_t getCapacity() const { return capacity_; }
  void setCapacity(std::uint16_t capacity) { capacity_ = capacity; }

  std::uint32_t getExperience() const { return experience_; }
  void setExperience(std::uint32_t experience) { experience_ = experience; }

  std::uint8_t getMagicLevel() const { return magicLevel_; }
  void setMagicLevel(std::uint8_t magicLevel) { magicLevel_ = magicLevel; }

  int getPartyShield() const { return partyShield_; }
  void setPartyShield(int partyShield) { partyShield_ = partyShield; }

  std::uint8_t getLevel() const;

  const Equipment& getEquipment() const
  {
    return equipment_;
  }

  Equipment& getEquipment()
  {
    return equipment_;
  }

 private:
  std::uint16_t maxMana_;
  std::uint16_t mana_;
  std::uint16_t capacity_;
  std::uint32_t experience_;
  std::uint8_t magicLevel_;
  int partyShield_;
  Equipment equipment_;
};

#endif  // GAMEENGINE_SRC_PLAYER_H_
