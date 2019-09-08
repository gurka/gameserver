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
  auto it = things_.cbegin() + 1;
  while (it != things_.cend())
  {
    // Iterate until we have passed all items with onTop = true
    // e.g. found a creature, an item with onTop = false, or end
    if (!it->isItem || !it->item.onTop)
    {
      break;
    }
    ++it;
  }
  things_.emplace(it, creatureId);
}

bool Tile::removeCreature(CreatureId creatureId)
{
  const auto it = std::find_if(things_.cbegin(),
                               things_.cend(),
                               [&creatureId](const Thing& t)
  {
    return !t.isItem && t.creatureId == creatureId;
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

CreatureId Tile::getCreatureId(int stackPosition) const
{
  if (stackPosition == 0 || static_cast<int>(things_.size()) < stackPosition)
  {
    LOG_ERROR("%s: invalid stackPosition: %d with things_.size(): %d",
              __func__,
              stackPosition,
              things_.size());
    return Creature::INVALID_ID;
  }

  if (things_[stackPosition].isItem)
  {
    LOG_ERROR("%s: thing on stackPosition: %d is an Item", __func__, stackPosition);
    return Creature::INVALID_ID;
  }

  return things_[stackPosition].creatureId;
}

int Tile::getCreatureStackPos(CreatureId creatureId) const
{
  const auto it = std::find_if(things_.cbegin(),
                               things_.cend(),
                               [&creatureId](const Thing& t)
  {
    return !t.isItem && t.creatureId == creatureId;
  });
  if (it == things_.cend())
  {
    LOG_ERROR("%s: no creature with id %d at this Tile", __func__, creatureId);
    return 255;  // TODO(simon): Invalid stackPosition constant?
  }
  return std::distance(things_.cbegin(), it);
}

void Tile::addItem(ItemUniqueId itemUniqueId, bool onTop)
{
  if (onTop)
  {
    things_.emplace(things_.cbegin() + 1, itemUniqueId, onTop);
  }
  else
  {
    auto it = things_.cbegin() + 1;
    while (it != things_.cend())
    {
      // Iterate until we have reached first item with onTop = false or end
      if (it->isItem && !it->item.onTop)
      {
        break;
      }
      ++it;
    }
    things_.emplace(it, itemUniqueId, onTop);
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

ItemUniqueId Tile::getItemUniqueId(int stackPosition) const
{
  if (stackPosition == 0 || static_cast<int>(things_.size()) < stackPosition)
  {
    LOG_ERROR("%s: invalid stackPosition: %d with things_.size(): %d",
              __func__,
              stackPosition,
              things_.size());
    return Item::INVALID_UNIQUE_ID;
  }

  if (!things_[stackPosition].isItem)
  {
    LOG_ERROR("%s: thing on stackPosition: %d is a Creature", __func__, stackPosition);
    return Item::INVALID_UNIQUE_ID;
  }

  return things_[stackPosition].item.itemUniqueId;
}
