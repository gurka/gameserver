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

#include <algorithm>

#include "logger.h"

namespace wsclient::wsworld
{

void Map::setFullMapData(const protocol::client::FullMap& map_data)
{
  // Full map data is width=18, height=14
  m_tiles.setMapPosition(map_data.position);
  auto it = map_data.tiles.begin();
  for (auto z = 0; z < m_tiles.getNumFloors(); z++)
  {
    for (auto x = 0; x < consts::KNOWN_TILES_X; x++)
    {
      for (auto y = 0; y < consts::KNOWN_TILES_Y; y++)
      {
        auto* tile = m_tiles.getTileLocalPos(x, y, z);
        setTile(*it, tile);
        ++it;
      }
    }
  }

  m_ready = true;
}

void Map::setPartialMapData(const protocol::client::PartialMap& map_data)
{
  // Set new map position
  const auto old_position = m_tiles.getMapPosition();
  const auto x_diff = map_data.direction == common::Direction::EAST ? 1 : (map_data.direction == common::Direction::WEST ? -1 : 0);
  const auto y_diff = map_data.direction == common::Direction::SOUTH ? 1 : (map_data.direction == common::Direction::NORTH ? -1 : 0);
  const auto new_position = common::Position(old_position.getX() + x_diff,
                                             old_position.getY() + y_diff,
                                             old_position.getZ());
  m_tiles.setMapPosition(new_position);
  LOG_INFO("%s: updated map position from %s to %s",
           __func__,
           old_position.toString().c_str(),
           new_position.toString().c_str());

  // Shift Tiles
  m_tiles.shiftTiles(map_data.direction);

  // Add new Tiles
  auto it = map_data.tiles.begin();
  for (auto z = 0; z < m_tiles.getNumFloors(); z++)
  {
    switch (map_data.direction)
    {
      case common::Direction::NORTH:
        for (auto x = 0; x < consts::KNOWN_TILES_X; x++)
        {
          auto* tile = m_tiles.getTileLocalPos(x, 0, z);
          setTile(*it, tile);
          ++it;
        }
        break;

      case common::Direction::EAST:
        for (auto y = 0; y < consts::KNOWN_TILES_Y; y++)
        {
          auto* tile = m_tiles.getTileLocalPos(consts::KNOWN_TILES_X - 1, y, z);
          setTile(*it, tile);
          ++it;
        }
        break;

      case common::Direction::SOUTH:
        for (auto x = 0; x < consts::KNOWN_TILES_X; x++)
        {
          auto* tile = m_tiles.getTileLocalPos(x, consts::KNOWN_TILES_Y - 1, z);
          setTile(*it, tile);
          ++it;
        }
        break;

      case common::Direction::WEST:
        for (auto y = 0; y < consts::KNOWN_TILES_Y; y++)
        {
          auto* tile = m_tiles.getTileLocalPos(0, y, z);
          setTile(*it, tile);
          ++it;
        }
        break;
    }
  }
}

void Map::updateTile(const protocol::client::TileUpdate& tile_update)
{
  auto* world_tile = m_tiles.getTile(tile_update.position);
  setTile(tile_update.tile, world_tile);
}

void Map::handleFloorChange(bool up, const protocol::client::FloorChange& floor_change)
{
  // Save number of floors _before_ changing map position
  const auto num_floors = m_tiles.getNumFloors();

  const auto current_position = m_tiles.getMapPosition();
  m_tiles.setMapPosition(common::Position(current_position.getX() + (up ? 1 : -1),
                                          current_position.getY() + (up ? 1 : -1),
                                          current_position.getZ() + (up ? -1 : 1)));


  if (up && m_tiles.getMapPosition().getZ() == 7)
  {
    // Moved up from underground to sea level
    // We have floors: 6 7 8 9 10
    // and received floors: 5 4 3 2 1 0
    // End result should be: 7 6 5 4 3 2 1 0
    // Swap floor[0] and floor[1], then insert new tiles at floor[2]
    m_tiles.swapFloors(0, 1);
    auto it = floor_change.tiles.cbegin();
    for (auto z = 2; z < 8; ++z)
    {
      for (auto x = 0; x < consts::KNOWN_TILES_X; ++x)
      {
        for (auto y = 0; y < consts::KNOWN_TILES_Y; ++y)
        {
          setTile(*it, m_tiles.getTileLocalPos(x, y, z));
          ++it;
        }
      }
    }
  }
  else if (up && m_tiles.getMapPosition().getZ() == 7)
  {
    // Move up from underground to underground
    // We have 5 to 3 floors (depending on old z)
    // and received one floor
    // Shift all floors forward one step (but max 5 floors)
    // and insert the new floor at [0]
    // e.g. from 12 13 14 15 to 11 12 13 14 15
    // or from 7 8 9 10 11 to 6 7 8 9 10
    m_tiles.shiftFloorForwards(num_floors);
    auto it = floor_change.tiles.cbegin();
    for (auto x = 0; x < consts::KNOWN_TILES_X; ++x)
    {
      for (auto y = 0; y < consts::KNOWN_TILES_Y; ++y)
      {
        setTile(*it, m_tiles.getTileLocalPos(x, y, 0));
        ++it;
      }
    }
  }
  else if (!up && m_tiles.getMapPosition().getZ() == 8)
  {
    // Moved down from sea level to underground
    // We have floors: 7 6 5 4 3 2 1 0
    // and received floors: 8 9 10 (order?)
    // End result should be: 6 7 8 9 10
    // Swap floor[0] and floor[1], then insert new tiles at floor[2]
    m_tiles.swapFloors(0, 1);
    auto it = floor_change.tiles.cbegin();
    for (auto z = 2; z < 5; ++z)
    {
      for (auto x = 0; x < consts::KNOWN_TILES_X; ++x)
      {
        for (auto y = 0; y < consts::KNOWN_TILES_Y; ++y)
        {
          setTile(*it, m_tiles.getTileLocalPos(x, y, z));
          ++it;
        }
      }
    }
  }
  else if (!up && m_tiles.getMapPosition().getZ() == 1)
  {
    // Moved down from underground to underground
    // We have 5 to 3 floors (depending on old z)
    // and received one or zero floors
    // Shift all floors backwards one step
    // and insert the new floor at the end (index depend on new z)
    // e.g. from 7 8 9 10 11 to 8 9 10 11 12
    // or 12 13 14 15 to 13 14 15
    m_tiles.shiftFloorBackwards(num_floors);
    auto it = floor_change.tiles.cbegin();
    for (auto x = 0; x < consts::KNOWN_TILES_X; ++x)
    {
      for (auto y = 0; y < consts::KNOWN_TILES_Y; ++y)
      {
        setTile(*it, m_tiles.getTileLocalPos(x, y, num_floors - 1));
        ++it;
      }
    }
  }
}

void Map::addProtocolThing(const common::Position& position, const protocol::Thing& thing)
{
  addThing(position, parseThing(thing));
}

void Map::addThing(const common::Position& position, Thing thing)
{
  auto& things = m_tiles.getTile(position)->things;

  const auto pre = things.size();

  auto it = things.cbegin() + 1;
  if (std::holds_alternative<Item>(thing))
  {
    const auto& item = std::get<Item>(thing);
    if (item.type->is_on_bottom)
    {
      // Insert before first on_top item or first creature
      while (it != things.cend())
      {
        if ((std::holds_alternative<Item>(*it) && std::get<Item>(*it).type->is_on_top) ||
            std::holds_alternative<common::CreatureId>(*it))
        {
          break;
        }
        ++it;
      }
    }
    else if (item.type->is_on_top)
    {
      // Insert before first creature or first item with neither is_on_bottom nor is_on_top
      while (it != things.cend())
      {
        if (std::holds_alternative<common::CreatureId>(*it) ||
            (std::holds_alternative<Item>(*it) && !std::get<Item>(*it).type->is_on_top && !std::get<Item>(*it).type->is_on_bottom))
        {
          break;
        }
        ++it;
      }
    }
    else
    {
      // Insert before first item with neither is_on_bottom nor is_on_top
      while (it != things.cend())
      {
        if (std::holds_alternative<Item>(*it) && !std::get<Item>(*it).type->is_on_top && !std::get<Item>(*it).type->is_on_bottom)
        {
          break;
        }
        ++it;
      }
    }
  }
  else
  {
    // Insert before first creature or first item with neither is_on_bottom nor is_on_top
    while (it != things.cend())
    {
      if (std::holds_alternative<common::CreatureId>(*it) ||
          (std::holds_alternative<Item>(*it) && !std::get<Item>(*it).type->is_on_top && !std::get<Item>(*it).type->is_on_bottom))
      {
        break;
      }
      ++it;
    }
  }

  it = things.insert(it, thing);
  if (things.size() > 10)
  {
    LOG_DEBUG("%s: Tile has more than 10 Things -> removing Thing at stackpos=10", __func__);
    things.erase(things.begin() + 10);
  }
  const auto post = things.size();

  (void)pre;
  (void)post;
#if 0
  LOG_INFO("%s: added Thing on position=%s stackpos=%d (size=%d->%d)",
           __func__,
           position.toString().c_str(),
           std::distance(things.cbegin(), it),
           pre,
           post);
#endif
}

void Map::removeThing(const common::Position& position, std::uint8_t stackpos)
{
  const auto pre = m_tiles.getTile(position)->things.size();

  auto& things = m_tiles.getTile(position)->things;
  if (things.size() <= stackpos)
  {
    // This might not be an error
    // It seems that the original server could send packets like this to the client
    LOG_ERROR("%s: no Thing at stackpos=%d, number of Things: %u", __func__, stackpos, things.size());
    return;
  }
  things.erase(things.cbegin() + stackpos);
  const auto post = m_tiles.getTile(position)->things.size();

  (void)pre;
  (void)post;
#if 0
  LOG_INFO("%s: removed thing from position=%s stackpos=%d (size=%d->%d)",
           __func__,
           position.toString().c_str(),
           stackpos,
           pre,
           post);
#endif
}

void Map::updateThing(const common::Position& position,
                      std::uint8_t stackpos,
                      const protocol::Thing& thing)
{
  m_tiles.getTile(position)->things[stackpos] = parseThing(thing);
}

void Map::moveThing(const common::Position& from_position,
                    std::uint8_t from_stackpos,
                    const common::Position& to_position)
{
  const auto thing = getThing(from_position, from_stackpos);
  if (!std::holds_alternative<common::CreatureId>(thing))
  {
    LOG_ERROR("%s: Thing is not Creature, from_pos=%s from_stackpos=%d to_pos=%s",
              __func__,
              from_position.toString().c_str(),
              from_stackpos,
              to_position.toString().c_str());

#if 0
    // Print all positions that have Creature(s) to debug what went wrong
    for (auto z = 0; z < m_tiles.getNumFloors(); z++)
    {
      for (auto y = 0; y < consts::KNOWN_TILES_Y; y++)
      {
        for (auto x = 0; x < consts::KNOWN_TILES_X; x++)
        {
          const auto* tile = m_tiles.getTileLocalPos(x, y, z);
          for (const auto& thing : tile->things)
          {
            if (std::holds_alternative<common::CreatureId>(thing))
            {
              const auto local_pos = common::Position(x, y, z);
              LOG_ERROR("%s: found Creature (%d) at local pos %s global pos %s",
                        __func__,
                        std::get<common::CreatureId>(thing),
                        local_pos.toString().c_str(),
                        m_tiles.localToGlobalPosition(local_pos).toString().c_str());
            }
          }
        }
      }
    }
#endif


    return;
  }

  removeThing(from_position, from_stackpos);

  // Rotate Creature based on movement
  // TODO(simon): handle diagonal movement
  auto* creature = getCreature(std::get<common::CreatureId>(thing));
  if (from_position.getX() > to_position.getX())
  {
    creature->direction = common::Direction::WEST;
  }
  else if (from_position.getX() < to_position.getX())
  {
    creature->direction = common::Direction::EAST;
  }
  else if (from_position.getY() > to_position.getY())
  {
    creature->direction = common::Direction::NORTH;
  }
  else if (from_position.getY() < to_position.getY())
  {
    creature->direction = common::Direction::SOUTH;
  }

  addThing(to_position, thing);

  //LOG_DEBUG("%s: moved Creature %d from %s to %s", __func__, creature->id, from_position.toString().c_str(), to_position.toString().c_str());
}

void Map::setCreatureSkull(common::CreatureId creature_id, std::uint8_t skull)
{
  auto* creature = getCreature(creature_id);
  if (!creature)
  {
    LOG_ERROR("%s: could not find known Creature with id %u", __func__, creature_id);
  }

  (void)skull;
  // TODO(simon): it->skull = skull;
}

const Tile* Map::getTile(const common::Position& position) const
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

Thing Map::parseThing(const protocol::Thing& thing)
{
  if (std::holds_alternative<protocol::Creature>(thing))
  {
    const auto& creature = std::get<protocol::Creature>(thing);
    if (creature.update != protocol::Creature::Update::NEW)
    {
      // FULL or DIRECTION
      auto* known_creature = getCreature(creature.id);
      if (!known_creature)
      {
        LOG_ERROR("%s: received creature id %u that is not known", __func__, creature.id);
        return Thing();
      }

      known_creature->direction = creature.direction;

      if (creature.update == protocol::Creature::Update::FULL)
      {
        known_creature->health_percent = creature.health_percent;
        known_creature->outfit = creature.outfit;
        known_creature->speed = creature.speed;
      }
    }
    else
    {
      // Remove known creature if set
      if (creature.id_to_remove != 0U)
      {
        auto it = std::find_if(m_known_creatures.begin(),
                               m_known_creatures.end(),
                               [id_to_remove = creature.id_to_remove](const Creature& creature)
        {
          return id_to_remove == creature.id;
        });
        if (it == m_known_creatures.end())
        {
          LOG_ERROR("%s: received CreatureId to remove %u but we do not know a Creature with that id",
                    __func__,
                    creature.id_to_remove);
        }
        else
        {
          LOG_DEBUG("%s: removing known Creature with id %u", __func__, creature.id_to_remove);
          m_known_creatures.erase(it);
        }
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

    LOG_INFO("%s: parsed creature with id %u", __func__, creature.id);

    return creature.id;
  }

  // Item
  const auto& protocol_item = std::get<protocol::Item>(thing);
  const auto& itemtype = (*m_itemtypes)[protocol_item.item_type_id];
  Item item;
  item.type = &itemtype;
  item.extra = protocol_item.extra;
  return item;
}

void Map::setTile(const protocol::Tile& protocol_tile, Tile* world_tile)
{
  // Note: this not care about stackpos, just adds the Things are they are in protocol_tile
  world_tile->things.clear();
  if (protocol_tile.skip)
  {
    return;
  }

  for (const auto& thing : protocol_tile.things)
  {
    world_tile->things.emplace_back(parseThing(thing));
  }
}

Creature* Map::getCreature(common::CreatureId creature_id)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* creature = static_cast<const Map*>(this)->getCreature(creature_id);
  return const_cast<Creature*>(creature);
}

Thing Map::getThing(const common::Position& position, std::uint8_t stackpos)
{
  return m_tiles.getTile(position)->things[stackpos];
}

}  // namespace wsclient::wsworld
