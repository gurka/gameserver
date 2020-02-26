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

void Map::setMapData(const protocol::client::Map& map_data)
{
  m_player_position = map_data.position;
  auto it = map_data.tiles.begin();
  for (auto x = 0; x < consts::known_tiles_x; x++)
  {
    for (auto y = 0; y < consts::known_tiles_y; y++)
    {
      auto& tile = m_tiles[y][x];
      tile.things.clear();

      if (!it->skip)
      {
        for (const auto& thing : it->things)
        {
          if (std::holds_alternative<protocol::Creature>(thing))
          {
            const auto& creature = std::get<protocol::Creature>(thing);
            if (creature.known)
            {
              // Update known creature
              auto* known_creature = getCreature(creature.id);
              if (!known_creature)
              {
                LOG_ERROR("%s: received known creature %u that is not known", __func__, creature.id);
                return;
              }
              known_creature->health_percent = creature.health_percent;
              known_creature->direction = creature.direction;
              known_creature->outfit = creature.outfit;
              known_creature->speed = creature.speed;
            }
            else
            {
              // Remove known creature if set
              if (creature.id_to_remove != 0U)
              {
                LOG_ERROR("%s: remove known creature not yet implemented", __func__);
              }

              // Add new creature
              m_known_creatures.emplace_back();
              m_known_creatures.back().id = creature.id;
              m_known_creatures.back().name = creature.name;
              m_known_creatures.back().health_percent = creature.health_percent;
              m_known_creatures.back().direction = creature.direction;
              m_known_creatures.back().outfit = creature.outfit;
              m_known_creatures.back().speed = creature.speed;
            }

            tile.things.emplace_back(creature.id);
          }
          else  // Item
          {
            const auto& item = std::get<protocol::Item>(thing);
            const auto& itemtype = (*m_itemtypes)[item.item_type_id];
            Item new_item;
            new_item.type = &itemtype;
            new_item.extra = item.extra;
            Thing t = new_item;
            tile.things.push_back(t);
          }
        }
      }
      ++it;
    }
  }

  m_ready = true;
}

void Map::addCreature(const common::Position& position, common::CreatureId creature_id)
{
  const auto x = position.getX() + 8 - m_player_position.getX();
  const auto y = position.getY() + 6 - m_player_position.getY();

  // Find first creature, bottom item or end
  auto it = m_tiles[y][x].things.cbegin() + 1;
  while (it != m_tiles[y][x].things.cend())
  {
    if (std::holds_alternative<common::CreatureId>(*it))
    {
      break;
    }
    else if (!std::get<Item>(*it).type->always_on_top)
    {
      break;
    }
    ++it;
  }

  // Add creature here
//  Tile::Thing thing;
//  thing.is_item = false;
//  thing.creature_id = creature_id;
//  it = m_tiles[y][x].things.insert(it, thing);

  LOG_INFO("%s: added creature_id=%d on position=%s stackpos=%d",
           __func__,
           creature_id,
           position.toString().c_str(),
           std::distance(m_tiles[y][x].things.cbegin(), it));
}

void Map::addCreature(const common::Position& position, const protocol::Creature& creature)
{
  addCreature(position, creature.id);
}

void Map::addItem(const common::Position& position,
                  common::ItemTypeId item_type_id,
                  std::uint8_t extra,
                  bool onTop)
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
      if (std::holds_alternative<Item>(*it) && !std::get<Item>(*it).type->always_on_top)
      {
        break;
      }
      ++it;
    }
  }

//  // Add item here
//  Tile::Thing thing;
//  thing.is_item = true;
//  thing.item.item_type_id = item_type_id;
//  thing.item.extra = extra;
//  thing.item.onTop = onTop;
//  it = m_tiles[y][x].things.insert(it, thing);

  LOG_INFO("%s: added item_type_id=%d on position=%s stackpos=%d",
           __func__,
           item_type_id,
           position.toString().c_str(),
           std::distance(m_tiles[y][x].things.cbegin(), it));
}

void Map::removeThing(const common::Position& position, std::uint8_t stackpos)
{
  const auto x = position.getX() + 8 - m_player_position.getX();
  const auto y = position.getY() + 6 - m_player_position.getY();

  m_tiles[y][x].things.erase(m_tiles[y][x].things.cbegin() + stackpos);

  LOG_INFO("%s: removed thing from position=%s stackpos=%d",
           __func__,
           position.toString().c_str(),
           stackpos);
}

const Map::Tile& Map::getTile(const common::Position& position) const
{
  const auto x = position.getX() + 8 - m_player_position.getX();
  const auto y = position.getY() + 6 - m_player_position.getY();

  return m_tiles[y][x];
}

const Map::Creature* Map::getCreature(common::CreatureId creature_id) const
{
  auto it = std::find_if(m_known_creatures.cbegin(),
                         m_known_creatures.cend(),
                         [creature_id](const Creature& creature)
  {
    return creature_id == creature.id;
  });
  if (it == m_known_creatures.cend())
  {
    return nullptr;
  }
  return &(*it);
}

Map::Creature* Map::getCreature(common::CreatureId creature_id)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* creature = static_cast<const Map*>(this)->getCreature(creature_id);
  return const_cast<Creature*>(creature);
}

}  // namespace wsclient::wsworld
