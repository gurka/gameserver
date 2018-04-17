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

#ifndef GAMEENGINE_EXPORT_GAME_POSITION_H_
#define GAMEENGINE_EXPORT_GAME_POSITION_H_

#include <cstdint>
#include <cstdio>
#include <string>

#include "position.h"

class GamePosition
{
 public:
  GamePosition()
    : type_(Type::INVALID)
  {
  }

  explicit GamePosition(const Position& position)
    : type_(Type::POSITION),
      position_(position)
  {
  }

  explicit GamePosition(int inventorySlot)
    : type_(Type::INVENTORY),
      inventorySlot_(inventorySlot)
  {
  }

  GamePosition(int containerId, int containerSlot)
    : type_(Type::CONTAINER),
      container_{containerId, containerSlot}
  {
  }

  std::string toString() const
  {
    if (type_ == Type::INVALID)
    {
      return "INVALID";
    }
    else if (type_ == Type::POSITION)
    {
      return std::string("(Position) ") + position_.toString();
    }
    else if (type_ == Type::INVENTORY)
    {
      return std::string("(Inventory) ") + std::to_string(inventorySlot_);
    }
    else  // type_ == Type::CONTAINER
    {
      return std::string("(Container) ") + std::to_string(container_.id) + ", " + std::to_string(container_.slot);
    }
  }

  bool isValid() const { return type_ != Type::INVALID; }

  bool isPosition() const { return type_ == Type::POSITION; }
  const Position& getPosition() const { return position_; }

  bool isInventory() const { return type_ == Type::INVENTORY; }
  int getInventorySlot() const { return inventorySlot_; }

  bool isContainer() const { return type_ == Type::CONTAINER; }
  int getContainerId() const { return container_.id; }
  int getContainerSlot() const { return container_.slot; }

 private:
  enum class Type
  {
    INVALID,
    POSITION,
    INVENTORY,
    CONTAINER,
  } type_;

  union
  {
    Position position_;
    int inventorySlot_;
    struct
    {
      int id;
      int slot;
    } container_;
  };
};

class ItemPosition
{
 public:
  ItemPosition()
    : gamePosition_(),
      itemId_(0),  // TODO(simon): Item::INVALID_ID ?
      stackPosition_(0)
  {
  }

  ItemPosition(const GamePosition& gamePosition, ItemId itemId, int stackPosition)
    : gamePosition_(gamePosition),
      itemId_(itemId),
      stackPosition_(stackPosition)
  {
  }

  std::string toString() const
  {
    return gamePosition_.toString() + ", " + std::to_string(itemId_) + ", " + std::to_string(stackPosition_);
  }

  const GamePosition& getGamePosition() const { return gamePosition_; }
  ItemId getItemId() const { return itemId_; }
  int getStackPosition() const { return stackPosition_; }

 private:
  GamePosition gamePosition_;
  ItemId itemId_;
  int stackPosition_;
};

#endif  // GAMEENGINE_EXPORT_GAME_POSITION_H_
