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

#include "logger.h"

void Tile::addThing(const Thing& thing)
{
  if (thing.creature)
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
    things_.insert(it, thing);
  }
  else  // thing.item
  {
    if (thing.item->getItemType().alwaysOnTop)
    {
      things_.insert(things_.cbegin() + 1, thing);
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
      things_.insert(it, thing);
    }
  }
}

bool Tile::removeThing(int stackPosition)
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

const Thing* Tile::getThing(int stackPosition) const
{
  if (static_cast<int>(things_.size()) < stackPosition)
  {
    LOG_ERROR("%s: invalid stackPosition: %d with things_.size(): %d",
              __func__,
              stackPosition,
              things_.size());
    return nullptr;
  }

  return &things_[stackPosition];
}
