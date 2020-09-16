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
#include "wsworld.h"

#include "logger.h"

namespace wsclient::wsworld
{

void Map::setMapData(const protocol::client::Map& map_data)
{
  // Full map data is width=18, height=14
  // The position we received is the point X below
  // This is the player position, and this gives us one
  // extra column to the right and one extra row at the bottom
  // . 0 1 2 3 4 5 6 7 8 9 a b c d e f g h
  // 0
  // 1
  // 2
  // 3
  // 4
  // 5
  // 6                 X
  // 7
  // 8
  // 9
  // a
  // b
  // c
  // d

  // Assume that when we receive full map that we also reset player position
  // to the received position
  m_tiles.setMapPosition(map_data.position);

  auto it = map_data.tiles.begin();
  for (auto x = map_data.position.getX() - 8; x < map_data.position.getX() + 10; x++)
  {
    for (auto y = map_data.position.getY() - 6; y < map_data.position.getY() + 8; y++)
    {
      auto& tile = m_tiles.getTile(common::Position(x, y, 0));
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

              if (creature.id == m_player_id)
              {
                LOG_INFO("%s: we are %s!", __func__, creature.name.c_str());
              }
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
  addThing(position, creature_id);
}

void Map::addCreature(const common::Position& position, const protocol::Creature& creature)
{
  // TODO: add to known creatures if needed?
  addCreature(position, creature.id);
}

void Map::addItem(const common::Position& position,
                  common::ItemTypeId item_type_id,
                  std::uint8_t extra,
                  bool onTop)
{
  // TODO: why is onTop sent here?
  (void)onTop;

  Item item;
  item.type = &((*m_itemtypes)[item_type_id]);
  item.extra = extra;
  addThing(position, item);
}

void Map::removeThing(const common::Position& position, std::uint8_t stackpos)
{
  const auto pre = m_tiles.getTile(position).things.size();
  auto& things = m_tiles.getTile(position).things;
  things.erase(things.cbegin() + stackpos);
  const auto post = m_tiles.getTile(position).things.size();

  LOG_INFO("%s: removed thing from position=%s stackpos=%d (size=%d->%d)",
           __func__,
           position.toString().c_str(),
           stackpos,
           pre,
           post);
}

void Map::moveThing(const common::Position& from_position,
                    std::uint8_t from_stackpos,
                    const common::Position& to_position)
{
  const auto thing = getThing(from_position, from_stackpos);
  removeThing(from_position, from_stackpos);
  addThing(to_position, thing);
}

const Tile& Map::getTile(const common::Position& position) const
{
  return m_tiles.getTile(position);
}

const Creature* Map::getCreature(common::CreatureId creature_id) const
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

Creature* Map::getCreature(common::CreatureId creature_id)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* creature = static_cast<const Map*>(this)->getCreature(creature_id);
  return const_cast<Creature*>(creature);
}

Thing Map::getThing(const common::Position& position, std::uint8_t stackpos)
{
  return m_tiles.getTile(position).things[stackpos];
}

void Map::addThing(const common::Position& position, Thing thing)
{
  auto& things = m_tiles.getTile(position).things;
  const auto pre = things.size();
  auto it = things.cbegin() + 1;
  if (std::holds_alternative<Item>(thing))
  {
    const auto& item = std::get<Item>(thing);
    if (item.type->always_on_top)
    {
      // Insert after ground item
      ++it;
    }
    else
    {
      // Find first bottom item or end
      auto it = things.cbegin();
      while (it != things.cend())
      {
        if (std::holds_alternative<Item>(*it) && !std::get<Item>(*it).type->always_on_top)
        {
          break;
        }
        ++it;
      }
    }
  }
  else
  {
    // Find first creature, bottom item or end
    while (it != things.cend())
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
  }

  it = things.insert(it, thing);
  const auto post = things.size();

  LOG_INFO("%s: added Thing on position=%s stackpos=%d (size=%d->%d)",
           __func__,
           position.toString().c_str(),
           std::distance(things.cbegin(), it),
           pre,
           post);
}

}  // namespace wsclient::wsworld
