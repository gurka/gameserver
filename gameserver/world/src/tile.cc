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
    auto it = m_things.cbegin() + 1;
    while (it != m_things.cend())
    {
      // Iterate until we have passed all items with onTop = true
      // e.g. found a creature, an item with onTop = false, or end
      if (!it->item || !it->item->getItemType().always_on_top)
      {
        break;
      }
      ++it;
    }
    m_things.insert(it, thing);
  }
  else  // thing.item
  {
    if (thing.item->getItemType().always_on_top)
    {
      m_things.insert(m_things.cbegin() + 1, thing);
    }
    else
    {
      auto it = m_things.cbegin() + 1;
      while (it != m_things.cend())
      {
        // Iterate until we have reached first item with onTop = false or end
        if (it->item && !it->item->getItemType().always_on_top)
        {
          break;
        }
        ++it;
      }
      m_things.insert(it, thing);
    }
  }
}

bool Tile::removeThing(int stackpos)
{
  if (stackpos == 0 || static_cast<int>(m_things.size()) < stackpos)
  {
    LOG_ERROR("%s: invalid stackpos: %d with m_things.size(): %d",
              __func__,
              stackpos,
              m_things.size());
    return false;
  }

  m_things.erase(m_things.cbegin() + stackpos);
  return true;
}

const Thing* Tile::getThing(int stackpos) const
{
  if (static_cast<int>(m_things.size()) < stackpos)
  {
    LOG_ERROR("%s: invalid stackpos: %d with m_things.size(): %d",
              __func__,
              stackpos,
              m_things.size());
    return nullptr;
  }

  return &m_things[stackpos];
}
