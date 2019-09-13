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

#include "world.h"

#include <algorithm>
#include <array>
#include <deque>
#include <random>
#include <sstream>
#include <tuple>
#include <utility>

#include "logger.h"
#include "tick.h"
#include "thing.h"

World::World(int world_size_x,
             int world_size_y,
             std::vector<Tile>&& tiles)
  : m_world_size_x(world_size_x),
    m_world_size_y(world_size_y),
    m_tiles(std::move(tiles))
{
}

World::~World() = default;

World::ReturnCode World::addCreature(Creature* creature, CreatureCtrl* creature_ctrl, const Position& position)
{
  auto creature_id = creature->getCreatureId();

  if (creatureExists(creature_id))
  {
    LOG_ERROR("%s: Creature already exists: %s (%d)",
              __func__,
              creature->getName().c_str(),
              creature->getCreatureId());
    return ReturnCode::OTHER_ERROR;
  }

  // Offsets for other possible positions
  // (0, 0) MUST be the first element
  static std::array<std::tuple<int, int>, 9> position_offsets
  {{
    { std::make_tuple( 0,  0) },
    { std::make_tuple(-1, -1) },
    { std::make_tuple(-1,  0) },
    { std::make_tuple(-1,  1) },
    { std::make_tuple( 0, -1) },
    { std::make_tuple( 0,  1) },
    { std::make_tuple( 1, -1) },
    { std::make_tuple( 1,  0) },
    { std::make_tuple( 1,  1) }
  }};

  // Shuffle the offsets (keep first element at its position)
  // TODO(simon): Have global random_device
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(position_offsets.begin() + 1, position_offsets.end(), g);

  auto adjusted_position = position;
  Tile* tile = nullptr;
  for (const auto& offsets : position_offsets)
  {
    adjusted_position = Position(position.getX() + std::get<0>(offsets),
                                position.getY() + std::get<1>(offsets),
                                position.getZ());

    tile = getTile(adjusted_position);
    if (!tile)
    {
      continue;
    }

    // Check if other creature already there
    const auto& things = tile->getThings();
    const auto it = std::find_if(things.cbegin(),
                                 things.cend(),
                                 [](const Thing& thing)
    {
      return thing.creature != nullptr;
    });
    if (it != things.cend())
    {
      tile = nullptr;
      continue;
    }

    break;
  }

  if (tile)
  {
    LOG_INFO("%s: spawning creature: %d at position: %s", __func__, creature_id, adjusted_position.toString().c_str());
    tile->addThing(Thing(creature));

    m_creature_data.emplace(std::piecewise_construct,
                           std::forward_as_tuple(creature_id),
                           std::forward_as_tuple(creature, creature_ctrl, adjusted_position));

    // Tell near creatures that a creature has spawned
    // Including the spawned creature!
    auto near_creature_ids = getCreatureIdsThatCanSeePosition(adjusted_position);
    for (const auto& near_creature_id : near_creature_ids)
    {
      getCreatureCtrl(near_creature_id).onCreatureSpawn(*creature, adjusted_position);
    }

    return ReturnCode::OK;
  }

  LOG_DEBUG("%s: could not find a tile around position %s to spawn creature: %d",
            __func__,
            position.toString().c_str(),
            creature_id);
  return ReturnCode::OTHER_ERROR;
}

void World::removeCreature(CreatureId creature_id)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("%s: called with non-existent CreatureId", __func__);
    return;
  }

  const auto& creature = getCreature(creature_id);
  const auto& position = getCreaturePosition(creature_id);
  auto* tile = getTile(position);
  const auto stackpos = getCreatureStackpos(position, creature_id);

  // Tell near creatures that a creature has despawned
  // Including the despawning creature!
  auto near_creature_ids = getCreatureIdsThatCanSeePosition(position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onCreatureDespawn(creature, position, stackpos);
  }

  m_creature_data.erase(creature_id);
  tile->removeThing(stackpos);
}

bool World::creatureExists(CreatureId creature_id) const
{
  return creature_id != Creature::INVALID_ID && m_creature_data.count(creature_id) == 1;
}

World::ReturnCode World::creatureMove(CreatureId creature_id, Direction direction)
{
  return creatureMove(creature_id, getCreaturePosition(creature_id).addDirection(direction));
}

World::ReturnCode World::creatureMove(CreatureId creature_id, const Position& to_position)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("%s: called with non-existent CreatureId", __func__);
    return ReturnCode::INVALID_CREATURE;
  }

  auto* to_tile = getTile(to_position);
  if (!to_tile)
  {
    LOG_ERROR("%s: no tile found at to_position: %s", __func__, to_position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Get Creature
  auto& creature = getCreature(creature_id);

  // Check if Creature may move at this time
  auto current_tick = Tick::now();
  if (creature.getNextWalkTick() > current_tick)
  {
    LOG_DEBUG("%s: current_tick = %d nextWalkTick = %d => MAY_NOT_MOVE_YET",
              __func__,
              current_tick,
              creature.getNextWalkTick());
    return ReturnCode::MAY_NOT_MOVE_YET;
  }

  // Check if to_tile is blocking or not
  for (const auto& thing : to_tile->getThings())
  {
    if (thing.item && thing.item->getItemType().is_blocking)
    {
      LOG_DEBUG("%s: Item on to_tile is blocking", __func__);
      return ReturnCode::THERE_IS_NO_ROOM;
    }

    if (thing.creature)
    {
      LOG_DEBUG("%s: Item on to_tile has creatures", __func__);
      return ReturnCode::THERE_IS_NO_ROOM;
    }
  }

  // Move the actual creature
  auto from_position = getCreaturePosition(creature_id);  // Need to create a new Position here (i.e. not auto&)
  auto* from_tile = getTile(from_position);
  auto from_stackpos = getCreatureStackpos(from_position, creature_id);
  from_tile->removeThing(from_stackpos);

  to_tile->addThing(m_creature_data.at(creature_id).creature);
  m_creature_data.at(creature_id).position = to_position;

  // Set new nextWalkTime for this Creature
  auto ground_speed = from_tile->getThing(0)->item->getItemType().speed;
  auto creature_speed = creature.getSpeed();
  auto duration = (1000 * ground_speed) / creature_speed;

  // Walking diagonally?
  if (from_position.getX() != to_position.getX() &&
      from_position.getY() != to_position.getY())
  {
    // Or is it times 3?
    duration *= 2;
  }

  creature.setNextWalkTick(current_tick + duration);

  // Update direction
  if (from_position.getY() > to_position.getY())
  {
    creature.setDirection(Direction::NORTH);
  }
  else if (from_position.getY() < to_position.getY())
  {
    creature.setDirection(Direction::SOUTH);
  }
  if (from_position.getX() > to_position.getX())
  {
    creature.setDirection(Direction::WEST);
  }
  else if (from_position.getX() < to_position.getX())
  {
    creature.setDirection(Direction::EAST);
  }

  // Call onCreatureMove on all creatures that can see the movement
  // including the moving creature itself
  // Note that the range of which we iterate over tiles, (-9, -7) to (+8, +6),
  // are the opposite of Protocol71::canSee (-8, -6) to (+9, +7)
  // This makes sense, as we here want to know "who can see us move?" while
  // in Protocol71::canSee we want to know "can we see them move?"
  // TODO(simon): refactor this and Protocol71::canSee to get rid of all these
  //              constant integers
  const auto x_min = std::min(from_position.getX(), to_position.getX());
  const auto x_max = std::max(from_position.getX(), to_position.getX());
  const auto y_min = std::min(from_position.getY(), to_position.getY());
  const auto y_max = std::max(from_position.getY(), to_position.getY());
  for (auto x = x_min - 9; x <= x_max + 8; x++)
  {
    for (auto y = y_min - 7; y <= y_max + 6; y++)
    {
      const auto position = Position(x, y, 7);
      const auto* tile = getTile(position);
      if (!tile)
      {
        continue;
      }
      for (const auto& thing : tile->getThings())
      {
        if (thing.creature)
        {
          getCreatureCtrl(thing.creature->getCreatureId()).onCreatureMove(creature,
                                                                          from_position,
                                                                          from_stackpos,
                                                                          to_position);
        }
      }
    }
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the from_tile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (from_tile->getNumberOfThings() >= 10)
  {
    auto near_creature_ids = getCreatureIdsThatCanSeePosition(from_position);
    for (const auto& near_creature_id : near_creature_ids)
    {
      getCreatureCtrl(near_creature_id).onTileUpdate(from_position);
    }
  }

  return ReturnCode::OK;
}

void World::creatureTurn(CreatureId creature_id, Direction direction)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("creatureTurn called with non-existent CreatureId");
    return;
  }

  auto& creature = getCreature(creature_id);
  creature.setDirection(direction);

  // Call onCreatureTurn on all creatures that can see the turn
  // including the turning creature itself
  const auto& position = getCreaturePosition(creature_id);
  const auto stackpos = getCreatureStackpos(position, creature_id);
  const auto near_creature_ids = getCreatureIdsThatCanSeePosition(position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onCreatureTurn(creature, position, stackpos);
  }
}

void World::creatureSay(CreatureId creature_id, const std::string& message)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("creatureSay called with non-existent CreatureId");
    return;
  }

  const auto& creature = getCreature(creature_id);
  const auto& position = getCreaturePosition(creature_id);
  auto near_creature_ids = getCreatureIdsThatCanSeePosition(position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onCreatureSay(creature, position, message);
  }
}

bool World::canAddItem(const Item& item, const Position& position) const
{
  // TODO(simon): are there cases where only specific items can be added to certain tiles?
  (void)item;

  const auto* tile = getTile(position);
  if (!tile)
  {
    LOG_ERROR("%s: no tile found at position: %s", __func__, position.toString().c_str());
    return false;
  }

  for (const auto& thing : tile->getThings())
  {
    if (thing.item && thing.item->getItemType().is_blocking)
    {
      return false;
    }
  }

  return true;
}

World::ReturnCode World::addItem(const Item& item, const Position& position)
{
  auto* tile = getTile(position);
  if (!tile)
  {
    LOG_ERROR("%s: no tile found at position: %s", __func__, position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Add Item to to_tile
  tile->addThing(&item);

  // Call onItemAdded on all creatures that can see position
  auto near_creature_ids = getCreatureIdsThatCanSeePosition(position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onItemAdded(item, position);
  }

  return ReturnCode::OK;
}

World::ReturnCode World::removeItem(ItemTypeId item_type_id, int count, const Position& position, int stackpos)
{
  // TODO(simon): implement count
  (void)count;

  auto* tile = getTile(position);
  if (!tile)
  {
    LOG_ERROR("%s: no tile found at position: %s", __func__, position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Get the item and verify item_type_id
  const auto* item = tile->getThing(stackpos)->item;
  if (!item || item->getItemTypeId() != item_type_id)
  {
    LOG_ERROR("%s: item with given stackpos does not match given item_type_id", __func__);
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Try to remove Item from the tile
  if (!tile->removeThing(stackpos))
  {
    LOG_ERROR("%s: could not remove item with item_type_id %d from %s",
              __func__,
              item_type_id,
              position.toString().c_str());
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Call onItemRemoved on all creatures that can see the position
  auto near_creature_ids = getCreatureIdsThatCanSeePosition(position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onItemRemoved(position, stackpos);
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the tile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (tile->getNumberOfThings() >= 10)
  {
    for (const auto& near_creature_id : near_creature_ids)
    {
      getCreatureCtrl(near_creature_id).onTileUpdate(position);
    }
  }

  return ReturnCode::OK;
}

World::ReturnCode World::moveItem(CreatureId creature_id,
                                  const Position& from_position,
                                  int from_stackpos,
                                  ItemTypeId item_type_id,
                                  int count,
                                  const Position& to_position)
{
  // TODO(simon): implement count
  (void)count;

  // TODO(simon): re-use addItem and removeItem

  if (!creatureExists(creature_id))
  {
    LOG_ERROR("%s: called with non-existent CreatureId", __func__);
    return ReturnCode::INVALID_CREATURE;
  }

  // Get tiles
  auto* from_tile = getTile(from_position);
  if (!from_tile)
  {
    LOG_ERROR("%s: could not find tile at position: %s",
              __func__,
              from_position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  auto* to_tile = getTile(to_position);
  if (!to_tile)
  {
    LOG_ERROR("%s: could not find tile at position: %s",
              __func__,
              to_position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Only allow move if the player is standing at or 1 sqm near the item
  const auto& position = getCreaturePosition(creature_id);
  if (std::abs(position.getX() - from_position.getX()) > 1 ||
      std::abs(position.getY() - from_position.getY()) > 1 ||
      position.getZ() != from_position.getZ())
  {
    LOG_DEBUG("%s: player is too far away", __func__);
    return ReturnCode::CANNOT_REACH_THAT_OBJECT;
  }

  // Check if we can add Item to to_tile
  for (const auto& thing : to_tile->getThings())
  {
    if (thing.item && thing.item->getItemType().is_blocking)
    {
      LOG_DEBUG("%s: Item on to_tile is blocking", __func__);
      return ReturnCode::THERE_IS_NO_ROOM;
    }
  }

  // Get the Item from from_tile
  auto* item = from_tile->getThing(from_stackpos)->item;
  if (!item || item->getItemTypeId() != item_type_id)
  {
    LOG_ERROR("%s: Could not find the item to move", __func__);
    return ReturnCode::ITEM_NOT_FOUND;
  }
  // Try to remove Item from from_tile
  if (!from_tile->removeThing(from_stackpos))
  {
    LOG_DEBUG("%s: Could not remove item with item_type_id %d from %s",
              __func__,
              item_type_id,
              from_position.toString().c_str());
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Add Item to to_tile
  to_tile->addThing(item);

  // Call onItemRemoved on all creatures that can see from_position
  const auto near_creature_ids_from = getCreatureIdsThatCanSeePosition(from_position);
  for (const auto& near_creature_id : near_creature_ids_from)
  {
    getCreatureCtrl(near_creature_id).onItemRemoved(from_position, from_stackpos);
  }

  // Call onItemAdded on all creatures that can see to_position
  const auto near_creature_ids_to = getCreatureIdsThatCanSeePosition(to_position);
  for (const auto& near_creature_id : near_creature_ids_to)
  {
    getCreatureCtrl(near_creature_id).onItemAdded(*item, to_position);
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the from_tile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (from_tile->getNumberOfThings() >= 10)
  {
    for (const auto& near_creature_id : near_creature_ids_from)
    {
      getCreatureCtrl(near_creature_id).onTileUpdate(from_position);
    }
  }

  return ReturnCode::OK;
}

bool World::creatureCanThrowTo(CreatureId creature_id, const Position& position) const
{
  // TODO(simon): Fix
  (void)creature_id;
  (void)position;
  return true;
}

bool World::creatureCanReach(CreatureId creature_id, const Position& position) const
{
  const auto& creature_position = getCreaturePosition(creature_id);
  return !(std::abs(creature_position.getX() - position.getX()) > 1 ||
           std::abs(creature_position.getY() - position.getY()) > 1 ||
           creature_position.getZ() != position.getZ());
}

const Tile* World::getTile(const Position& position) const
{
  if (position.getX() < POSITION_OFFSET ||
      position.getX() >= POSITION_OFFSET + m_world_size_x ||
      position.getY() < POSITION_OFFSET ||
      position.getY() >= POSITION_OFFSET + m_world_size_y ||
      position.getZ() != 7)
  {
    return nullptr;
  }

  const auto index = ((position.getX() - POSITION_OFFSET) * m_world_size_y) +
                      (position.getY() - POSITION_OFFSET);
  return &m_tiles[index];
}

const Creature& World::getCreature(CreatureId creature_id) const
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("%s: called with non-existent CreatureId: %d", __func__, creature_id);
    return Creature::INVALID;
  }
  return *(m_creature_data.at(creature_id).creature);
}

const Position& World::getCreaturePosition(CreatureId creature_id) const
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("getCreaturePosition called with non-existent CreatureId");
    return Position::INVALID;
  }
  return m_creature_data.at(creature_id).position;
}

Tile* World::getTile(const Position& position)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* tile = static_cast<const World*>(this)->getTile(position);
  return const_cast<Tile*>(tile);
}

Creature& World::getCreature(CreatureId creature_id)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto& creature = static_cast<const World*>(this)->getCreature(creature_id);
  return const_cast<Creature&>(creature);
}

CreatureCtrl& World::getCreatureCtrl(CreatureId creature_id)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("getCreatureCtrl called with non-existent CreatureId");
  }
  return *(m_creature_data.at(creature_id).creature_ctrl);
}

std::vector<CreatureId> World::getCreatureIdsThatCanSeePosition(const Position& position) const
{
  std::vector<CreatureId> creature_ids;

  // TODO(simon): fix these constants (see creatureMove)
  for (int x = position.getX() - 9; x <= position.getX() + 8; ++x)
  {
    for (int y = position.getY() - 7; y <= position.getY() + 6; ++y)
    {
      const auto* tile = getTile(Position(x, y, position.getZ()));
      if (!tile)
      {
        continue;
      }

      for (const auto& thing : tile->getThings())
      {
        if (thing.creature)
        {
          creature_ids.push_back(thing.creature->getCreatureId());
        }
      }
    }
  }

  return creature_ids;
}

int World::getCreatureStackpos(const Position& position, CreatureId creature_id) const
{
  const auto* tile = getTile(position);
  if (!tile)
  {
    return 255;  // TODO(simon): invalid stackpos
  }
  const auto& things = tile->getThings();
  const auto it = std::find_if(things.cbegin(),
                               things.cend(),
                               [&creature_id](const Thing& thing)
  {
    return thing.creature && thing.creature->getCreatureId() == creature_id;  // NOLINT
  });
  if (it != things.cend())
  {
    return std::distance(things.cbegin(), it);
  }

  LOG_ERROR("%s: could not find creature_id: %d in Tile: %s",
            __func__,
            creature_id,
            position.toString().c_str());
  return 255;  // TODO(simon): invalid stackpos
}
