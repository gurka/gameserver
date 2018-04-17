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

#include "tile.h"

#include <algorithm>
#include <iterator>

#include "logger.h"

void Tile::addCreature(CreatureId creatureId)
{
  creatureIds_.insert(creatureIds_.begin(), creatureId);
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
  int index = stackPosition - 1 - numberOfTopItems;
  if (index < 0 || index >= static_cast<int>(creatureIds_.size()))
  {
    LOG_ERROR("%s: No Creature found at stackPosition: %d", __func__, stackPosition);
    return Creature::INVALID_ID;
  }

  auto creatureIt = creatureIds_.cbegin();
  std::advance(creatureIt, index);
  return *creatureIt;
}

int Tile::getCreatureStackPos(CreatureId creatureId) const
{
  // Find where in creatureIds_ the creatureId is
  auto it = std::find(creatureIds_.cbegin(), creatureIds_.cend(), creatureId);
  if (it == creatureIds_.cend())
  {
    LOG_ERROR("getCreatureStackPos(): No creature %d at this Tile", creatureId);
    return 255;  // TODO(simon): Invalid stackPosition constant?
  }
  return 1 + numberOfTopItems + std::distance(creatureIds_.cbegin(), it);
}

void Tile::addItem(Item* item)
{
  auto itemIt = items_.cbegin();

  if (item->getItemType().alwaysOnTop)
  {
    std::advance(itemIt, 1);
    numberOfTopItems++;
  }
  else
  {
    std::advance(itemIt, 1 + numberOfTopItems);
  }

  items_.insert(itemIt, item);
}

bool Tile::removeItem(ItemTypeId itemTypeId, int stackPosition)
{
  if (stackPosition == 0)
  {
    // Ground Item
    LOG_ERROR("%s: Stackposition is ground Item, cannot remove", __func__);
  }
  else if (stackPosition < 1 + numberOfTopItems)
  {
    // Top Item
    auto itemIt = items_.cbegin();
    std::advance(itemIt, stackPosition);
    if ((*itemIt)->getItemTypeId() == itemTypeId)
    {
      items_.erase(itemIt);
      return true;
    }
    else
    {
      LOG_ERROR("%s: Given ItemTypeId does not match Item at given stackpos", __func__);
    }
  }
  else if (stackPosition < 1 + numberOfTopItems + static_cast<int>(creatureIds_.size()))
  {
    // Creature
    LOG_ERROR("%s: Stackposition is Creature, cannot remove", __func__);
  }
  else if (stackPosition < 1 + static_cast<int>(items_.size() + creatureIds_.size()))
  {
    // Bottom Item
    auto itemIt = items_.cbegin();
    std::advance(itemIt, stackPosition - creatureIds_.size());
    if ((*itemIt)->getItemTypeId() == itemTypeId)
    {
      items_.erase(itemIt);
      return true;
    }
    else
    {
      LOG_ERROR("%s: Given ItemTypeId does not match Item at given stackpos", __func__);
    }
  }
  else
  {
    // Invalid stackpos
    LOG_ERROR("%s: Stackposition is invalid", __func__);
  }

  return false;
}

const Item* Tile::getItem(int stackPosition) const
{
  if (stackPosition == 0)
  {
    // Ground Item
    return items_.front();
  }
  else if (stackPosition < 1 + numberOfTopItems)
  {
    // Top Item
    auto itemIt = items_.cbegin();
    std::advance(itemIt, stackPosition);
    return *itemIt;
  }
  else if (stackPosition < 1 + numberOfTopItems + static_cast<int>(creatureIds_.size()))
  {
    // Creature
    LOG_ERROR("%s: Stackposition is Creature", __func__);
    return nullptr;
  }
  else if (stackPosition < 1 + static_cast<int>(items_.size() + creatureIds_.size()))
  {
    // Bottom Item
    auto itemIt = items_.cbegin();
    std::advance(itemIt, stackPosition - creatureIds_.size());
    return *itemIt;
  }
  else
  {
    // Invalid stackpos
    LOG_ERROR("%s: Stackposition is invalid", __func__);
    return nullptr;
  }
}

Item* Tile::getItem(int stackPosition)
{
  const auto* item = static_cast<const Tile*>(this)->getItem(stackPosition);
  return const_cast<Item*>(item);
}

std::size_t Tile::getNumberOfThings() const
{
  return items_.size() + creatureIds_.size();
}

int Tile::getGroundSpeed() const
{
  if (items_.empty())
  {
    LOG_ERROR("%s: Tile does not have any items", __func__);
    return 0;
  }

  return items_.front()->getItemType().speed;
}
