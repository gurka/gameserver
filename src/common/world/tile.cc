/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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

#include "tile.h"

#include <algorithm>

#include "logger.h"

void Tile::addCreature(CreatureId creatureId)
{
  creatureIds_.push_front(creatureId);
}

bool Tile::removeCreature(CreatureId creatureId)
{
  auto it = std::find(creatureIds_.cbegin(), creatureIds_.cend(), creatureId);
  if (it != creatureIds_.cend())
  {
    creatureIds_.erase(it);
    return true;
  }
  else
  {
    return false;
  }
}

CreatureId Tile::getCreatureId(int stackPosition) const
{
  // Calculate position in creatureIds_
  auto position = stackPosition - 1 - topItems_.size();
  if (position < 0 || position >= creatureIds_.size())
  {
    LOG_ERROR("%s: No Creature found at stackPosition: %d", __func__, stackPosition);
    return Creature::INVALID_ID;
  }
  return creatureIds_[position];
}

uint8_t Tile::getCreatureStackPos(CreatureId creatureId) const
{
  // Find where in creatureIds_ the creatureId is
  auto it = std::find(creatureIds_.cbegin(), creatureIds_.cend(), creatureId);
  if (it == creatureIds_.cend())
  {
    LOG_ERROR("getCreatureStackPos(): No creature %d at this Tile", creatureId);
    return 255;  // TODO(gurka): Invalid stackPosition constant?
  }
  return 1 + topItems_.size() + std::distance(creatureIds_.cbegin(), it);
}

void Tile::addItem(const Item& item)
{
  if (item.alwaysOnTop())
  {
    topItems_.push_front(item);
  }
  else
  {
    bottomItems_.push_front(item);
  }
}

bool Tile::removeItem(const Item& item, uint8_t stackPosition)
{
  auto index = stackPosition;
  if (index == 0)
  {
    LOG_ERROR("removeItem(): Stackposition is ground item, cannot remove");
    return false;
  }

  index -= 1;
  if (index < topItems_.size())
  {
    auto it = topItems_.cbegin() + index;
    if (it->getItemId() == item.getItemId())
    {
      topItems_.erase(it);
      return true;
    }
    else
    {
      return false;
    }
  }

  index -= topItems_.size();
  if (index < creatureIds_.size())
  {
    LOG_ERROR("removeItem(): Stackposition is Creature, cannot remove");
    return false;
  }

  index -= creatureIds_.size();
  if (index < bottomItems_.size())
  {
    auto it = bottomItems_.cbegin() + index;
    if (it->getItemId() == item.getItemId())
    {
      bottomItems_.erase(it);
      return true;
    }
    else
    {
      return false;
    }
  }

  LOG_ERROR("removeItem(): Stackposition is invalid");
  return false;
}
/*
const Item* Tile::getItem(uint8_t stackPos) const
{
  auto index = stackPos;
  if (index == 0)
  {
    return &groundItem_;
  }

  index -= 1;
  if (index < topItems_.size())
  {
    return &topItems_.at(index);
  }

  index -= topItems_.size();
  if (index < creatureIds_.size())
  {
    LOG_ERROR("getItem(): Stackposition is a Creature");
    return nullptr;
  }

  index -= creatureIds_.size();
  if (index < bottomItems_.size())
  {
    return &bottomItems_.at(index);
  }

  LOG_ERROR("getItem(): Stackposition is invalid");
  return nullptr;
}

CreatureId Tile::getCreature(uint8_t stackPos) const
{
  auto index = stackPos;
  if (index == 0)  // Ground
  {
    LOG_ERROR("getItem(): Stackposition is an Item");
    return Creature::INVALID_ID;
  }

  index -= 1;
  if (index < topItems_.size())
  {
    LOG_ERROR("getItem(): Stackposition is an Item");
    return Creature::INVALID_ID;
  }

  index -= topItems_.size();
  if (index < creatureIds_.size())
  {
    return creatureIds_.at(index);
  }

  index -= creatureIds_.size();
  if (index < bottomItems_.size())
  {
    LOG_ERROR("getItem(): Stackposition is an Item");
    return Creature::INVALID_ID;
  }

  LOG_ERROR("getItem(): Stackposition is invalid");
  return Creature::INVALID_ID;
}
*/
std::size_t Tile::getNumberOfThings() const
{
  return 1 + topItems_.size() + creatureIds_.size() + bottomItems_.size();
}
