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

#ifndef GAMEENGINE_EXPORT_PLAYER_H_
#define GAMEENGINE_EXPORT_PLAYER_H_

#include <array>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>

#include "creature.h"
#include "direction.h"
#include "item.h"

namespace gameengine
{

class Equipment
{
 public:
  Equipment()
    : m_items()
  {
  }

  bool canAddItem(const common::Item& item, int inventory_slot) const;
  bool addItem(const common::Item& item, int inventory_slot);
  bool removeItem(common::ItemTypeId item_type_id, int inventory_slot);
  const common::Item* getItem(int inventory_slot) const;

 private:
  std::array<const common::Item*, 11> m_items;  // index 0 is invalid
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

class Player : public common::Creature
{
 public:
  Player(common::CreatureId creature_id, const std::string& name);

  // From Creature
  std::uint16_t getSpeed() const override;

  std::uint16_t getMaxMana() const { return m_max_mana; }
  void setMaxMana(std::uint16_t max_mana) { m_max_mana = max_mana; }

  std::uint16_t getMana() const { return m_mana; }
  void setMana(std::uint16_t mana) { m_mana = mana; }

  std::uint16_t getCapacity() const { return m_capacity; }
  void setCapacity(std::uint16_t capacity) { m_capacity = capacity; }

  std::uint32_t getExperience() const { return m_experience; }
  void setExperience(std::uint32_t experience) { m_experience = experience; }

  std::uint8_t getMagicLevel() const { return m_magic_level; }
  void setMagicLevel(std::uint8_t magic_level) { m_magic_level = magic_level; }

  int getPartyShield() const { return m_party_shield; }
  void setPartyShield(int party_shield) { m_party_shield = party_shield; }

  std::uint8_t getLevel() const;

  const Equipment& getEquipment() const
  {
    return m_equipment;
  }

  Equipment& getEquipment()
  {
    return m_equipment;
  }

 private:
  std::uint16_t m_max_mana;
  std::uint16_t m_mana;
  std::uint16_t m_capacity;
  std::uint32_t m_experience;
  std::uint8_t m_magic_level;
  int m_party_shield;
  Equipment m_equipment;
};

}  // namespace gameengine

#endif  // GAMEENGINE_EXPORT_PLAYER_H_
