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
#include "wsworld.h"

#include "logger.h"

namespace wsclient::wsworld
{

void Map::setMapData(const protocol::client::MapData& map_data)
{
  m_player_position = map_data.position;
  auto it = map_data.tiles.begin();
  for (auto x = 0; x < consts::known_tiles_x; x++)
  {
    for (auto y = 0; y < consts::known_tiles_y; y++)
    {
      m_tiles[y][x].things.clear();

      if (!it->skip)
      {
        // Find largest stackpos
        const auto maxStackpos = std::max(it->items.empty() ? 0 : it->items.back().stackpos,
                                          it->creatures.empty() ? 0 : it->creatures.back().stackpos);

        // Make things vector this big
        m_tiles[y][x].things.resize(maxStackpos + 1);

        // Add items
        for (const auto& item : it->items)
        {
          //LOG_INFO("Adding an item at stackpos=%d with item_type_id=%d", item.stackpos, item.item.item_type_id);
          m_tiles[y][x].things[item.stackpos].is_item = true;
          m_tiles[y][x].things[item.stackpos].item.item_type_id = item.item.item_type_id;
          m_tiles[y][x].things[item.stackpos].item.extra = item.item.extra;
          m_tiles[y][x].things[item.stackpos].item.onTop = false;  // TODO fix
        }

        // Add creatures
        for (const auto& creature : it->creatures)
        {
          LOG_INFO("Adding a creature at stackpos=%d with creature_id=%d", creature.stackpos, creature.creature.id);
          m_tiles[y][x].things[creature.stackpos].is_item = false;
          m_tiles[y][x].things[creature.stackpos].creature_id = creature.creature.id;
        }
      }
      ++it;
    }
  }
}

void Map::addCreature(const Position& position, CreatureId creature_id)
{
  const auto x = position.getX() + 8 - m_player_position.getX();
  const auto y = position.getY() + 6 - m_player_position.getY();

  // Find first creature, bottom item or end
  auto it = m_tiles[y][x].things.cbegin() + 1;
  while (it != m_tiles[y][x].things.cend())
  {
    if (!it->is_item || !it->item.onTop)
    {
      break;
    }
    ++it;
  }

  // Add creature here
  Tile::Thing thing;
  thing.is_item = false;
  thing.creature_id = creature_id;
  it = m_tiles[y][x].things.insert(it, thing);

  LOG_INFO("%s: added creature_id=%d on position=%s stackpos=%d",
           __func__,
           creature_id,
           position.toString().c_str(),
           std::distance(m_tiles[y][x].things.cbegin(), it));
}

void Map::addItem(const Position& position, ItemTypeId item_type_id, std::uint8_t extra, bool onTop)
{
  const auto x = position.getX() + 8 - m_player_position.getX();
  const auto y = position.getY() + 6 - m_player_position.getY();

  auto it = m_tiles[y][x].things.cbegin() + 1;
  if (onTop)
  {
    // Insert after ground item
    ++it;
  }
  else
  {
    // Find first bottom item or end
    auto it = m_tiles[y][x].things.cbegin();
    while (it != m_tiles[y][x].things.cend())
    {
      if (it->is_item && !it->item.onTop)
      {
        break;
      }
      ++it;
    }
  }

  // Add item here
  Tile::Thing thing;
  thing.is_item = true;
  thing.item.item_type_id = item_type_id;
  thing.item.extra = extra;
  thing.item.onTop = onTop;
  it = m_tiles[y][x].things.insert(it, thing);

  LOG_INFO("%s: added item_type_id=%d on position=%s stackpos=%d",
           __func__,
           item_type_id,
           position.toString().c_str(),
           std::distance(m_tiles[y][x].things.cbegin(), it));
}

void Map::removeThing(const Position& position, std::uint8_t stackpos)
{
  const auto x = position.getX() + 8 - m_player_position.getX();
  const auto y = position.getY() + 6 - m_player_position.getY();

  m_tiles[y][x].things.erase(m_tiles[y][x].things.cbegin() + stackpos);

  LOG_INFO("%s: removed thing from position=%s stackpos=%d",
           __func__,
           position.toString().c_str(),
           stackpos);
}

const Map::Tile& Map::getTile(const Position& position) const
{
  const auto x = position.getX() + 8 - m_player_position.getX();
  const auto y = position.getY() + 6 - m_player_position.getY();

  return m_tiles[y][x];
}

}  // namespace wsclient::wsworld
