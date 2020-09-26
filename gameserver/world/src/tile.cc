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

#include "tile.h"

#include <type_traits>

#include "logger.h"

namespace world
{

void Tile::addThing(const common::Thing& thing)
{
  // Add the new thing before the first thing with the
  // same or greater prio than the new thing
  // 0 = item top
  // 1 = creature
  // 2 = item bottom
  // TODO(simon): not correct anymore? see wsclient/graphics.cc
  const auto thing_prio = thing.item() ? (thing.item()->getItemType().is_on_top ? 1 : 3) : 2;
  auto it = m_things.cbegin() + 1;
  while (it != m_things.cend())
  {
    const auto it_prio = it->item() ? (it->item()->getItemType().is_on_top ? 1 : 3) : 2;
    if (it_prio >= thing_prio)
    {
      break;
    }
    ++it;
  }
  m_things.insert(it, thing);
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

const common::Creature* Tile::getCreature(int stackpos) const
{
  if (static_cast<int>(m_things.size()) < stackpos)
  {
    LOG_ERROR("%s: invalid stackpos: %d with m_things.size(): %d",
              __func__,
              stackpos,
              m_things.size());
    return nullptr;
  }

  return m_things[stackpos].creature();
}

const common::Item* Tile::getItem(int stackpos) const
{
  if (static_cast<int>(m_things.size()) < stackpos)
  {
    LOG_ERROR("%s: invalid stackpos: %d with m_things.size(): %d",
              __func__,
              stackpos,
              m_things.size());
    return nullptr;
  }

  return m_things[stackpos].item();
}

bool Tile::isBlocking() const
{
  for (const auto& thing : m_things)
  {
    if (thing.hasCreature())
    {
      return true;
    }

    if (thing.item()->getItemType().is_blocking)
    {
      return true;
    }
  }
  return false;
}

int Tile::getCreatureStackpos(common::CreatureId creature_id) const
{
  auto it = m_things.cbegin() + 1;
  while (it != m_things.cend())
  {
    if (it->creature() && it->creature()->getCreatureId() == creature_id)
    {
      break;
    }
    ++it;
  }

  if (it == m_things.cend())
  {
    return 255;  // TODO(simon): invalid stackpos?
  }

  return std::distance(m_things.cbegin(), it);
}

}  // namespace world
