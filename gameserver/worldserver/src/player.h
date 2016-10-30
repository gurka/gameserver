/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
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

#ifndef WORLDSERVER_PLAYER_H_
#define WORLDSERVER_PLAYER_H_

#include <unordered_map>
#include <string>
#include "creature.h"
#include "direction.h"
#include "item.h"

class Equipment
{
 public:
  enum class Slot : uint8_t
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
    AMMO       = 10
  };

  Equipment();

  bool hasItem(uint8_t inventoryIndex) const
  {
    return hasItem(static_cast<Slot>(inventoryIndex));
  }

  bool hasItem(Slot slot) const
  {
    return items_.count(slot) == 1;
  }

  const Item& getItem(uint8_t inventoryIndex) const
  {
    return getItem(static_cast<Slot>(inventoryIndex));
  }
  const Item& getItem(Slot slot) const
  {
    return items_.at(slot);
  }

  bool addItem(const Item& item, uint8_t inventoryIndex);
  bool removeItem(const Item& item, uint8_t inventoryIndex);

  bool canAddItem(const Item& item, uint8_t inventoryIndex) const;

 private:
  struct SlotHash
  {
    std::size_t operator()(const Slot& slot) const
    {
      return static_cast<std::size_t>(slot);
    }
  };

  std::unordered_map<Slot, Item, SlotHash> items_;
};

class Player : public Creature
{
 public:
  explicit Player(const std::string& name);

  // From Creature
  int getSpeed() const override;


  int getMaxMana() const { return maxMana_; }
  void setMaxMana(int maxMana) { maxMana_ = maxMana; }

  int getMana() const { return mana_; }
  void setMana(int mana) { mana_ = mana; }

  int getCapacity() const { return capacity_; }
  void setCapacity(int capacity) { capacity_ = capacity; }

  int getExperience() const { return experience_; }
  void setExperience(int experience) { experience_ = experience; }

  int getMagicLevel() const { return magicLevel_; }
  void setMagicLevel(int magicLevel) { magicLevel_ = magicLevel; }

  int getPartyShield() const { return partyShield_; }
  void setPartyShield(int partyShield) { partyShield_ = partyShield; }

  int getLevel() const;

  const Equipment& getEquipment() const
  {
    return equipment_;
  }

  Equipment& getEquipment()
  {
    return equipment_;
  }

 private:
  int maxMana_;
  int mana_;
  int capacity_;
  int experience_;
  int magicLevel_;
  int partyShield_;
  Equipment equipment_;
};

#endif  // WORLDSERVER_PLAYER_H_
