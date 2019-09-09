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

void Tile::addCreature(const Creature* creature)
{
  auto it = things_.cbegin() + 1;
  while (it != things_.cend())
  {
    // Iterate until we have passed all items with onTop = true
    // e.g. found a creature, an item with onTop = false, or end
    if (!it->item || !it->item->getItemType().alwaysOnTop)
    {
      break;
    }
    ++it;
  }
  things_.emplace(it, creature);
}

bool Tile::removeCreature(CreatureId creatureId)
{
  const auto it = std::find_if(things_.cbegin(),
                               things_.cend(),
                               [&creatureId](const Thing& t)
  {
    return t.creature && t.creature->getCreatureId() == creatureId;
  });

  if (it != things_.cend())
  {
    things_.erase(it);
    return true;
  }
  else
  {
    return false;
  }
}

const Creature* Tile::getCreature(int stackPosition) const
{
  if (stackPosition == 0 || static_cast<int>(things_.size()) < stackPosition)
  {
    LOG_ERROR("%s: invalid stackPosition: %d with things_.size(): %d",
              __func__,
              stackPosition,
              things_.size());
    return nullptr;
  }

  if (!things_[stackPosition].creature)
  {
    LOG_ERROR("%s: thing on stackPosition: %d is an Item", __func__, stackPosition);
    return nullptr;
  }

  return things_[stackPosition].creature;
}

CreatureId Tile::getCreatureId(int stackPosition) const
{
  const auto* creature = getCreature(stackPosition);
  if (creature)
  {
    return creature->getCreatureId();
  }
  return Creature::INVALID_ID;
}

int Tile::getCreatureStackPos(CreatureId creatureId) const
{
  const auto it = std::find_if(things_.cbegin(),
                               things_.cend(),
                               [&creatureId](const Thing& t)
  {
    return t.creature && t.creature->getCreatureId() == creatureId;
  });
  if (it == things_.cend())
  {
    LOG_ERROR("%s: no creature with id %d at this Tile", __func__, creatureId);
    return 255;  // TODO(simon): Invalid stackPosition constant?
  }
  return std::distance(things_.cbegin(), it);
}

void Tile::addItem(const Item& item)
{
  if (item.getItemType().alwaysOnTop)
  {
    things_.emplace(things_.cbegin() + 1, &item);
  }
  else
  {
    auto it = things_.cbegin() + 1;
    while (it != things_.cend())
    {
      // Iterate until we have reached first item with onTop = false or end
      if (it->item && !it->item->getItemType().alwaysOnTop)
      {
        break;
      }
      ++it;
    }
    things_.emplace(it, &item);
  }
}

bool Tile::removeItem(int stackPosition)
{
  if (stackPosition == 0 || static_cast<int>(things_.size()) < stackPosition)
  {
    LOG_ERROR("%s: invalid stackPosition: %d with things_.size(): %d",
              __func__,
              stackPosition,
              things_.size());
    return false;
  }

  things_.erase(things_.cbegin() + stackPosition);
  return true;
}

const Item* Tile::getItem(int stackPosition) const
{
  if (stackPosition < 0 || static_cast<int>(things_.size()) < stackPosition)
  {
    LOG_ERROR("%s: invalid stackPosition: %d with things_.size(): %d",
              __func__,
              stackPosition,
              things_.size());
    return nullptr;
  }

  if (!things_[stackPosition].item)
  {
    LOG_ERROR("%s: thing on stackPosition: %d is a Creature", __func__, stackPosition);
    return nullptr;
  }

  return things_[stackPosition].item;
}

ItemUniqueId Tile::getItemUniqueId(int stackPosition) const
{
  const auto* item = getItem(stackPosition);
  if (item)
  {
    return item->getItemUniqueId();
  }
  return Item::INVALID_UNIQUE_ID;
}
