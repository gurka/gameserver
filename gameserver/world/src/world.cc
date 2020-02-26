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

#include "world.h"

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <random>
#include <sstream>
#include <tuple>
#include <utility>

#include "logger.h"
#include "tick.h"

namespace world
{

World::World(int world_size_x,
             int world_size_y,
             std::vector<Tile>&& tiles)
    : m_world_size_x(world_size_x),
      m_world_size_y(world_size_y),
      m_tiles(std::move(tiles))
{
}

World::~World() = default;

ReturnCode World::addCreature(common::Creature* creature, CreatureCtrl* creature_ctrl, const common::Position& position)
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
    adjusted_position = common::Position(position.getX() + std::get<0>(offsets),
                                 position.getY() + std::get<1>(offsets),
                                 position.getZ());

    tile = getTile(adjusted_position);
    if (!tile)
    {
      continue;
    }

    if (tile->isBlocking())
    {
      tile = nullptr;
      continue;
    }
    break;
  }

  if (tile)
  {
    LOG_INFO("%s: spawning creature: %d at position: %s", __func__, creature_id, adjusted_position.toString().c_str());
    tile->addThing(creature);

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

void World::removeCreature(common::CreatureId creature_id)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("%s: called with non-existent CreatureId", __func__);
    return;
  }

  const auto* creature = getCreature(creature_id);
  if (!creature)
  {
    LOG_ERROR("%s: invalid creature id: %u", __func__, creature_id);
    return;
  }
  const auto* position = getCreaturePosition(creature_id);
  if (!position)
  {
    LOG_ERROR("%s: invalid creature position", __func__);
    return;
  }
  auto* tile = getTile(*position);
  if (!tile)
  {
    LOG_ERROR("%s: invalid tile", __func__);
    return;
  }
  const auto stackpos = getCreatureStackpos(*position, creature_id);

  // Tell near creatures that a creature has despawned
  // Including the despawning creature!
  auto near_creature_ids = getCreatureIdsThatCanSeePosition(*position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onCreatureDespawn(*creature, *position, stackpos);
  }

  m_creature_data.erase(creature_id);
  tile->removeThing(stackpos);
}

bool World::creatureExists(common::CreatureId creature_id) const
{
  return creature_id != common::Creature::INVALID_ID && m_creature_data.count(creature_id) == 1;
}

ReturnCode World::creatureMove(common::CreatureId creature_id, common::Direction direction)
{
  const auto* position = getCreaturePosition(creature_id);
  if (!position)
  {
    LOG_ERROR("%s: invalid position", __func__);
    return ReturnCode::INVALID_POSITION;
  }
  return creatureMove(creature_id, position->addDirection(direction));
}

ReturnCode World::creatureMove(common::CreatureId creature_id, const common::Position& to_position)
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
  auto* creature = getCreature(creature_id);
  if (!creature)
  {
    LOG_ERROR("%s: invalid creature id: %u", __func__, creature_id);
    return ReturnCode::INVALID_CREATURE;
  }

  // Check if Creature may move at this time
  auto current_tick = utils::Tick::now();
  if (creature->getNextWalkTick() > current_tick)
  {
    LOG_DEBUG("%s: current_tick = %d nextWalkTick = %d => MAY_NOT_MOVE_YET",
              __func__,
              current_tick,
              creature->getNextWalkTick());
    return ReturnCode::MAY_NOT_MOVE_YET;
  }

  // Check if to_tile is blocking or not
  if (to_tile->isBlocking())
  {
    LOG_DEBUG("%s: to_tile is blocking", __func__);
    return ReturnCode::THERE_IS_NO_ROOM;
  }

  // Move the actual creature
  const auto* tmp_position = getCreaturePosition(creature_id);
  if (!tmp_position)
  {
    LOG_ERROR("%s: invalid position", __func__);
    return ReturnCode::INVALID_POSITION;
  }
  // Copy the old position
  const auto from_position = *tmp_position;
  auto* from_tile = getTile(from_position);
  auto from_stackpos = getCreatureStackpos(from_position, creature_id);
  from_tile->removeThing(from_stackpos);

  to_tile->addThing(creature);
  m_creature_data.at(creature_id).position = to_position;

  // Set new nextWalkTime for this Creature
  auto ground_speed = from_tile->getItem(0)->getItemType().speed;
  auto creature_speed = creature->getSpeed();
  auto duration = (1000 * ground_speed) / creature_speed;

  // Walking diagonally?
  if (from_position.getX() != to_position.getX() &&
      from_position.getY() != to_position.getY())
  {
    // Or is it times 3?
    duration *= 2;
  }

  creature->setNextWalkTick(current_tick + duration);

  // Update direction
  if (from_position.getY() > to_position.getY())
  {
    creature->setDirection(common::Direction::NORTH);
  }
  else if (from_position.getY() < to_position.getY())
  {
    creature->setDirection(common::Direction::SOUTH);
  }
  if (from_position.getX() > to_position.getX())
  {
    creature->setDirection(common::Direction::WEST);
  }
  else if (from_position.getX() < to_position.getX())
  {
    creature->setDirection(common::Direction::EAST);
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
      const auto position = common::Position(x, y, 7);
      const auto* tile = getTile(position);
      if (!tile)
      {
        continue;
      }

      for (const auto& thing : tile->getThings())
      {
        if (thing.hasCreature())
        {
          const auto creature_id = thing.creature()->getCreatureId();
          getCreatureCtrl(creature_id).onCreatureMove(*creature,
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

void World::creatureTurn(common::CreatureId creature_id, common::Direction direction)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("creatureTurn called with non-existent CreatureId");
    return;
  }

  auto* creature = getCreature(creature_id);
  if (!creature)
  {
    LOG_ERROR("%s: invalid creature id: %u", __func__, creature_id);
    return;
  }
  creature->setDirection(direction);

  // Call onCreatureTurn on all creatures that can see the turn
  // including the turning creature itself
  const auto* position = getCreaturePosition(creature_id);
  if (!position)
  {
    LOG_ERROR("%s: invalid position", __func__);
    return;
  }
  const auto stackpos = getCreatureStackpos(*position, creature_id);
  const auto near_creature_ids = getCreatureIdsThatCanSeePosition(*position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onCreatureTurn(*creature, *position, stackpos);
  }
}

void World::creatureSay(common::CreatureId creature_id, const std::string& message)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("creatureSay called with non-existent CreatureId");
    return;
  }

  const auto* creature = getCreature(creature_id);
  if (!creature)
  {
    LOG_ERROR("%s: invalid creature id: %u", __func__, creature_id);
    return;
  }
  const auto* position = getCreaturePosition(creature_id);
  if (!position)
  {
    LOG_ERROR("%s: invalid position", __func__);
    return;
  }
  auto near_creature_ids = getCreatureIdsThatCanSeePosition(*position);
  for (const auto& near_creature_id : near_creature_ids)
  {
    getCreatureCtrl(near_creature_id).onCreatureSay(*creature, *position, message);
  }
}

const common::Position* World::getCreaturePosition(common::CreatureId creature_id) const
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("getCreaturePosition called with non-existent CreatureId");
    return nullptr;
  }
  return &(m_creature_data.at(creature_id).position);
}

bool World::creatureCanThrowTo(common::CreatureId creature_id, const common::Position& position) const
{
  // TODO(simon): Fix
  (void)this;
  (void)creature_id;
  (void)position;
  return true;
}

bool World::creatureCanReach(common::CreatureId creature_id, const common::Position& position) const
{
  const auto* creature_position = getCreaturePosition(creature_id);
  return creature_position &&  // NOLINT readability-implicit-bool-conversion
         !(std::abs(creature_position->getX() - position.getX()) > 1 ||
           std::abs(creature_position->getY() - position.getY()) > 1 ||
           creature_position->getZ() != position.getZ());
}

bool World::canAddItem(const common::Item& item, const common::Position& position) const
{
  // TODO(simon): are there cases where only specific items can be added to certain tiles?
  (void)item;

  const auto* tile = getTile(position);
  if (!tile)
  {
    LOG_ERROR("%s: no tile found at position: %s", __func__, position.toString().c_str());
    return false;
  }

  return !tile->isBlocking();
}

ReturnCode World::addItem(const common::Item& item, const common::Position& position)
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

ReturnCode World::removeItem(common::ItemTypeId item_type_id, int count, const common::Position& position, int stackpos)
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
  const auto* item = tile->getItem(stackpos);
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

ReturnCode World::moveItem(common::CreatureId creature_id,
                                  const common::Position& from_position,
                                  int from_stackpos,
                                  common::ItemTypeId item_type_id,
                                  int count,
                                  const common::Position& to_position)
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
  const auto* position = getCreaturePosition(creature_id);
  if (!position)
  {
    LOG_ERROR("%s: invalid position", __func__);
    return ReturnCode::INVALID_POSITION;
  }
  if (std::abs(position->getX() - from_position.getX()) > 1 ||
      std::abs(position->getY() - from_position.getY()) > 1 ||
      position->getZ() != from_position.getZ())
  {
    LOG_DEBUG("%s: player is too far away", __func__);
    return ReturnCode::CANNOT_REACH_THAT_OBJECT;
  }

  // Check if we can add Item to to_tile
  if (to_tile->isBlocking())
  {
    LOG_DEBUG("%s: to_tile is blocking", __func__);
    return ReturnCode::THERE_IS_NO_ROOM;
  }

  // Get the Item from from_tile
  auto* item = from_tile->getItem(from_stackpos);
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

const Tile* World::getTile(const common::Position& position) const
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

Tile* World::getTile(const common::Position& position)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto* tile = static_cast<const World*>(this)->getTile(position);
  return const_cast<Tile*>(tile);
}

common::Creature* World::getCreature(common::CreatureId creature_id)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("%s: called with non-existent CreatureId: %d", __func__, creature_id);
    return nullptr;
  }
  return m_creature_data.at(creature_id).creature;
}

CreatureCtrl& World::getCreatureCtrl(common::CreatureId creature_id)
{
  if (!creatureExists(creature_id))
  {
    LOG_ERROR("getCreatureCtrl called with non-existent CreatureId");
  }
  return *(m_creature_data.at(creature_id).creature_ctrl);
}

std::vector<common::CreatureId> World::getCreatureIdsThatCanSeePosition(const common::Position& position) const
{
  std::vector<common::CreatureId> creature_ids;

  // TODO(simon): fix these constants (see creatureMove)
  for (int x = position.getX() - 9; x <= position.getX() + 8; ++x)
  {
    for (int y = position.getY() - 7; y <= position.getY() + 6; ++y)
    {
      const auto* tile = getTile(common::Position(x, y, position.getZ()));
      if (!tile)
      {
        continue;
      }

      for (const auto& thing : tile->getThings())
      {
        if (thing.hasCreature())
        {
          creature_ids.push_back(thing.creature()->getCreatureId());
        }
      }
    }
  }

  return creature_ids;
}

int World::getCreatureStackpos(const common::Position& position, common::CreatureId creature_id) const
{
  const auto* tile = getTile(position);
  if (!tile)
  {
    return 255;  // TODO(simon): invalid stackpos
  }

  return tile->getCreatureStackpos(creature_id);
}

}  // namespace world
