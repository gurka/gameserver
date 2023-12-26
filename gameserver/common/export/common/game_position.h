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

#ifndef COMMON_EXPORT_GAME_POSITION_H_
#define COMMON_EXPORT_GAME_POSITION_H_

#include <cstdint>
#include <cstdio>
#include <string>

#include "position.h"
#include "item.h"

namespace common
{

class GamePosition
{
 public:
  GamePosition()  // NOLINT due to bug in clang-tidy
  {
  }

  explicit GamePosition(const Position& position)
    : m_type(Type::POSITION)
  {
    m_up.position = position;
  }

  explicit GamePosition(int inventory_slot)
    : m_type(Type::INVENTORY)
  {
    m_up.inventory_slot = inventory_slot;
  }

  GamePosition(ItemUniqueId item_unique_id, int container_slot)
    : m_type(Type::CONTAINER)
  {
    m_up.container.item_unique_id = item_unique_id;
    m_up.container.slot = container_slot;
  }

  bool operator==(const GamePosition& other) const
  {
    // TODO(simon): test this
    return m_type == other.m_type &&
           ((m_type == Type::INVALID) ||  // should INVALID positions be equal?
            (m_type == Type::POSITION && m_up.position == other.m_up.position) ||
            (m_type == Type::INVENTORY && m_up.inventory_slot == other.m_up.inventory_slot) ||
            (m_type == Type::CONTAINER &&
             m_up.container.item_unique_id == other.m_up.container.item_unique_id &&
             m_up.container.slot == other.m_up.container.slot));
  }

  bool operator!=(const GamePosition& other) const
  {
    return !(*this == other);
  }

  std::string toString() const
  {
    if (m_type == Type::INVALID)
    {
      return "INVALID";
    }

    if (m_type == Type::POSITION)
    {
      return std::string("(Position) ") + m_up.position.toString();
    }

    if (m_type == Type::INVENTORY)
    {
      return std::string("(Inventory) ") + std::to_string(m_up.inventory_slot);
    }

    // m_type == Type::CONTAINER
    return std::string("(Container) ") +
           std::to_string(m_up.container.item_unique_id) + ", " + std::to_string(m_up.container.slot);
  }

  bool isValid() const { return m_type != Type::INVALID; }

  bool isPosition() const { return m_type == Type::POSITION; }
  const Position& getPosition() const { return m_up.position; }

  bool isInventory() const { return m_type == Type::INVENTORY; }
  int getInventorySlot() const { return m_up.inventory_slot; }

  bool isContainer() const { return m_type == Type::CONTAINER; }
  ItemUniqueId getItemUniqueId() const { return m_up.container.item_unique_id; }
  int getContainerSlot() const { return m_up.container.slot; }

 private:
  enum class Type
  {
    INVALID,
    POSITION,
    INVENTORY,
    CONTAINER,
  } m_type{Type::INVALID};

  union
  {
    Position position = { 0, 0, 0 };
    int inventory_slot;
    struct
    {
      ItemUniqueId item_unique_id;
      int slot;
    } container;
  } m_up;
};

class ItemPosition
{
 public:
  ItemPosition()
    : m_item_type_id(0)  // TODO(simon): Item::INVALID_ID ?
  {
  }

  // TODO(simon): stackpos is only used for GamePosition Type::POSITION, or??
  ItemPosition(const GamePosition& game_position, ItemTypeId item_type_id)
    : m_game_position(game_position),
      m_item_type_id(item_type_id)
  {
  }

  ItemPosition(const GamePosition& game_position, ItemTypeId item_type_id, int stackpos)
    : m_game_position(game_position),
      m_item_type_id(item_type_id),
      m_stackpos(stackpos)
  {
  }

  bool operator==(const ItemPosition& other) const
  {
    return m_game_position  == other.m_game_position &&
           m_item_type_id    == other.m_item_type_id &&
           m_stackpos == other.m_stackpos;
  }

  bool operator!=(const ItemPosition& other) const
  {
    return !(*this == other);
  }

  std::string toString() const
  {
    return m_game_position.toString() + ", " + std::to_string(m_item_type_id) + ", " + std::to_string(m_stackpos);
  }

  const GamePosition& getGamePosition() const { return m_game_position; }
  ItemTypeId getItemTypeId() const { return m_item_type_id; }
  int getStackPosition() const { return m_stackpos; }

 private:
  GamePosition m_game_position;
  ItemTypeId m_item_type_id{0};
  int m_stackpos{0};
};

}  // namespace common

#endif  // COMMON_EXPORT_GAME_POSITION_H_
