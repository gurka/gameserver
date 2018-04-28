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
#include <sstream>
#include <tuple>
#include <utility>

#include "logger.h"
#include "tick.h"

World::World(int worldSizeX,
             int worldSizeY,
             std::vector<Tile>&& tiles)
  : worldSizeX_(worldSizeX),
    worldSizeY_(worldSizeY),
    tiles_(std::move(tiles))
{
}

World::ReturnCode World::addCreature(Creature* creature, CreatureCtrl* creatureCtrl, const Position& position)
{
  auto creatureId = creature->getCreatureId();

  if (creatureExists(creatureId))
  {
    LOG_ERROR("%s: Creature already exists: %s (%d)",
              __func__,
              creature->getName().c_str(),
              creature->getCreatureId());
    return ReturnCode::OTHER_ERROR;
  }

  // Offsets for other possible positions
  // (0, 0) MUST be the first element
  static std::array<std::tuple<int, int>, 9> positionOffsets
  {{
    { std::make_tuple( 0,  0) },  //NOLINT
    { std::make_tuple(-1, -1) },  //NOLINT
    { std::make_tuple(-1,  0) },  //NOLINT
    { std::make_tuple(-1,  1) },  //NOLINT
    { std::make_tuple( 0, -1) },  //NOLINT
    { std::make_tuple( 0,  1) },  //NOLINT
    { std::make_tuple( 1, -1) },  //NOLINT
    { std::make_tuple( 1,  0) },  //NOLINT
    { std::make_tuple( 1,  1) }   //NOLINT
  }};

  // Shuffle the offsets (keep first element at its position)
  std::random_shuffle(positionOffsets.begin() + 1, positionOffsets.end());

  auto adjustedPosition = position;
  Tile* tile = nullptr;
  for (const auto& offsets : positionOffsets)
  {
    adjustedPosition = Position(position.getX() + std::get<0>(offsets),
                                position.getY() + std::get<1>(offsets),
                                position.getZ());

    tile = internalGetTile(adjustedPosition);
    if (!tile)
    {
      continue;
    }

    // TODO(simon): Need to check more stuff (blocking, etc)
    if (!tile->getCreatureIds().empty())
    {
      tile = nullptr;
      continue;
    }

    break;
  }

  if (tile)
  {
    LOG_INFO("%s: spawning creature: %d at position: %s", __func__, creatureId, adjustedPosition.toString().c_str());
    tile->addCreature(creatureId);

    creature_data_.emplace(std::piecewise_construct,
                           std::forward_as_tuple(creatureId),
                           std::forward_as_tuple(creature, creatureCtrl, adjustedPosition));

    // Tell near creatures that a creature has spawned
    // Including the spawned creature!
    auto nearCreatureIds = getCreatureIdsThatCanSeePosition(adjustedPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onCreatureSpawn(*this, *creature, adjustedPosition);
    }

    return ReturnCode::OK;
  }
  else
  {
    LOG_DEBUG("%s: could not find a tile around position %s to spawn creature: %d",
              __func__,
              position.toString().c_str(),
              creatureId);
    return ReturnCode::OTHER_ERROR;
  }
}

void World::removeCreature(CreatureId creatureId)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("%s: called with non-existent CreatureId", __func__);
    return;
  }

  const auto& creature = getCreature(creatureId);
  const auto& position = getCreaturePosition(creatureId);
  auto* tile = internalGetTile(position);
  auto stackPos = tile->getCreatureStackPos(creatureId);

  // Tell near creatures that a creature has despawned
  // Including the despawning creature!
  auto nearCreatureIds = getCreatureIdsThatCanSeePosition(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onCreatureDespawn(*this, creature, position, stackPos);
  }

  creature_data_.erase(creatureId);
  tile->removeCreature(creatureId);
}

bool World::creatureExists(CreatureId creatureId) const
{
  return creatureId != Creature::INVALID_ID && creature_data_.count(creatureId) == 1;
}

World::ReturnCode World::creatureMove(CreatureId creatureId, Direction direction)
{
  return creatureMove(creatureId, getCreaturePosition(creatureId).addDirection(direction));
}

World::ReturnCode World::creatureMove(CreatureId creatureId, const Position& toPosition)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("%s: called with non-existent CreatureId", __func__);
    return ReturnCode::INVALID_CREATURE;
  }

  auto* toTile = internalGetTile(toPosition);
  if (!toTile)
  {
    LOG_ERROR("%s: no tile found at toPosition: %s", __func__, toPosition.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Get Creature
  auto& creature = internalGetCreature(creatureId);

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

  // Check if toTile is blocking or not
  for (const auto* item : toTile->getItems())
  {
    if (item->getItemType().isBlocking)
    {
      LOG_DEBUG("%s: Item on toTile is blocking", __func__);
      return ReturnCode::THERE_IS_NO_ROOM;
    }

    if (!toTile->getCreatureIds().empty())
    {
      LOG_DEBUG("%s: Item on toTile has creatures", __func__);
      return ReturnCode::THERE_IS_NO_ROOM;
    }
  }

  // Move the actual creature
  auto fromPosition = getCreaturePosition(creatureId);  // Need to create a new Position here (i.e. not auto&)
  auto* fromTile = internalGetTile(fromPosition);
  auto fromStackPos = fromTile->getCreatureStackPos(creatureId);
  fromTile->removeCreature(creatureId);

  toTile->addCreature(creatureId);
  creature_data_.at(creatureId).position = toPosition;

  // Set new nextWalkTime for this Creature
  auto groundSpeed = fromTile->getGroundSpeed();
  auto creatureSpeed = creature.getSpeed();
  auto duration = (1000 * groundSpeed) / creatureSpeed;

  // Walking diagonally?
  if (fromPosition.getX() != toPosition.getX() &&
      fromPosition.getY() != toPosition.getY())
  {
    // Or is it times 3?
    duration *= 2;
  }

  creature.setNextWalkTick(current_tick + duration);

  // Update direction
  if (fromPosition.getY() > toPosition.getY())
  {
    creature.setDirection(Direction::NORTH);
  }
  else if (fromPosition.getY() < toPosition.getY())
  {
    creature.setDirection(Direction::SOUTH);
  }
  if (fromPosition.getX() > toPosition.getX())
  {
    creature.setDirection(Direction::WEST);
  }
  else if (fromPosition.getX() < toPosition.getX())
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
  const auto x_min = std::min(fromPosition.getX(), toPosition.getX());
  const auto x_max = std::max(fromPosition.getX(), toPosition.getX());
  const auto y_min = std::min(fromPosition.getY(), toPosition.getY());
  const auto y_max = std::max(fromPosition.getY(), toPosition.getY());
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
      const auto& nearCreatureIds = tile->getCreatureIds();
      for (const auto nearCreatureId : nearCreatureIds)
      {
        getCreatureCtrl(nearCreatureId).onCreatureMove(*this, creature, fromPosition, fromStackPos, toPosition);
      }
    }
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the fromTile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (fromTile->getNumberOfThings() >= 10)
  {
    auto nearCreatureIds = getCreatureIdsThatCanSeePosition(fromPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onTileUpdate(*this, fromPosition);
    }
  }

  return ReturnCode::OK;
}

void World::creatureTurn(CreatureId creatureId, Direction direction)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("creatureTurn called with non-existent CreatureId");
    return;
  }

  auto& creature = internalGetCreature(creatureId);
  creature.setDirection(direction);

  // Call onCreatureTurn on all creatures that can see the turn
  // including the turning creature itself
  const auto& position = getCreaturePosition(creatureId);
  auto stackPos = getTile(position)->getCreatureStackPos(creatureId);
  auto nearCreatureIds = getCreatureIdsThatCanSeePosition(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onCreatureTurn(*this, creature, position, stackPos);
  }
}

void World::creatureSay(CreatureId creatureId, const std::string& message)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("creatureSay called with non-existent CreatureId");
    return;
  }

  const auto& creature = getCreature(creatureId);
  const auto& position = getCreaturePosition(creatureId);
  auto nearCreatureIds = getCreatureIdsThatCanSeePosition(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onCreatureSay(*this, creature, position, message);
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

  for (const auto* item : tile->getItems())
  {
    if (item->getItemType().isBlocking)
    {
      return false;
    }
  }

  return true;
}

World::ReturnCode World::addItem(Item* item, const Position& position)
{
  auto* tile = internalGetTile(position);
  if (!tile)
  {
    LOG_ERROR("%s: no tile found at position: %s", __func__, position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Add Item to toTile
  tile->addItem(item);

  // Call onItemAdded on all creatures that can see position
  auto nearCreatureIds = getCreatureIdsThatCanSeePosition(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemAdded(*this, *item, position);
  }

  return ReturnCode::OK;
}

World::ReturnCode World::removeItem(ItemTypeId itemTypeId, int count, const Position& position, int stackPos)
{
  // TODO(simon): implement count
  (void)count;

  auto* tile = internalGetTile(position);
  if (!tile)
  {
    LOG_ERROR("%s: no tile found at position: %s", __func__, position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Try to remove Item from the tile
  if (!tile->removeItem(itemTypeId, stackPos))
  {
    LOG_ERROR("%s: could not remove item with itemTypeId %d from %s",
              __func__,
              itemTypeId,
              position.toString().c_str());
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Call onItemRemoved on all creatures that can see the position
  auto nearCreatureIds = getCreatureIdsThatCanSeePosition(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemRemoved(*this, position, stackPos);
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the tile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (tile->getNumberOfThings() >= 10)
  {
    auto nearCreatureIds = getCreatureIdsThatCanSeePosition(position);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onTileUpdate(*this, position);
    }
  }

  return ReturnCode::OK;
}

World::ReturnCode World::moveItem(CreatureId creatureId,
                                  const Position& fromPosition,
                                  int fromStackPos,
                                  ItemTypeId itemTypeId,
                                  int count,
                                  const Position& toPosition)
{
  // TODO(simon): implement count
  (void)count;

  // TODO(simon): re-use addItem and removeItem

  if (!creatureExists(creatureId))
  {
    LOG_ERROR("%s: called with non-existent CreatureId", __func__);
    return ReturnCode::INVALID_CREATURE;
  }

  // Get tiles
  auto* fromTile = internalGetTile(fromPosition);
  if (!fromTile)
  {
    LOG_ERROR("%s: could not find tile at position: %s",
              __func__,
              fromPosition.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  auto* toTile = internalGetTile(toPosition);
  if (!toTile)
  {
    LOG_ERROR("%s: could not find tile at position: %s",
              __func__,
              toPosition.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Only allow move if the player is standing at or 1 sqm near the item
  const auto& position = getCreaturePosition(creatureId);
  if (std::abs(position.getX() - fromPosition.getX()) > 1 ||
      std::abs(position.getY() - fromPosition.getY()) > 1 ||
      position.getZ() != fromPosition.getZ())
  {
    LOG_DEBUG("%s: player is too far away", __func__);
    return ReturnCode::CANNOT_REACH_THAT_OBJECT;
  }

  // Check if we can add Item to toTile
  for (const auto* item : toTile->getItems())
  {
    if (item->getItemType().isBlocking)
    {
      LOG_DEBUG("%s: Item on toTile is blocking", __func__);
      return ReturnCode::THERE_IS_NO_ROOM;
    }
  }

  // Get the Item from fromTile
  auto* item = fromTile->getItem(fromStackPos);
  if (!item)
  {
    LOG_ERROR("%s: Could not find the item to move", __func__);
    return ReturnCode::ITEM_NOT_FOUND;
  }
  // Try to remove Item from fromTile
  if (!fromTile->removeItem(itemTypeId, fromStackPos))
  {
    LOG_DEBUG("%s: Could not remove item with itemTypeId %d from %s",
              __func__,
              itemTypeId,
              fromPosition.toString().c_str());
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Add Item to toTile
  toTile->addItem(item);

  // Call onItemRemoved on all creatures that can see fromPosition
  auto nearCreatureIds = getCreatureIdsThatCanSeePosition(fromPosition);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemRemoved(*this, fromPosition, fromStackPos);
  }

  // Call onItemAdded on all creatures that can see toPosition
  nearCreatureIds = getCreatureIdsThatCanSeePosition(toPosition);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemAdded(*this, *item, toPosition);
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the fromTile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (fromTile->getNumberOfThings() >= 10)
  {
    auto nearCreatureIds = getCreatureIdsThatCanSeePosition(fromPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onTileUpdate(*this, fromPosition);
    }
  }

  return ReturnCode::OK;
}

Item* World::getItem(const Position& position, int stackPosition)
{
  auto* tile = internalGetTile(position);
  if (!tile)
  {
    LOG_ERROR("%s: could not find tile at position: %s", __func__, position.toString().c_str());
    return nullptr;
  }
  return tile->getItem(stackPosition);
}

bool World::creatureCanThrowTo(CreatureId creatureId, const Position& position) const
{
  // TODO(simon): Fix
  (void)creatureId;
  (void)position;
  return true;
}

bool World::creatureCanReach(CreatureId creatureId, const Position& position) const
{
  const auto& creaturePosition = getCreaturePosition(creatureId);
  return !(std::abs(creaturePosition.getX() - position.getX()) > 1 ||
           std::abs(creaturePosition.getY() - position.getY()) > 1 ||
           creaturePosition.getZ() != position.getZ());
}

std::vector<CreatureId> World::getCreatureIdsThatCanSeePosition(const Position& position) const
{
  std::vector<CreatureId> creatureIds;

  // TODO(simon): fix these constants (see creatureMove)
  for (int x = position.getX() - 9; x <= position.getX() + 8; ++x)
  {
    for (int y = position.getY() - 7; y <= position.getY() + 6; ++y)
    {
      Position tempPosition(x, y, position.getZ());
      const auto* tile = getTile(tempPosition);
      if (!tile)
      {
        continue;
      }

      if (!tile->getCreatureIds().empty())
      {
        creatureIds.insert(creatureIds.cend(),
                           tile->getCreatureIds().cbegin(),
                           tile->getCreatureIds().cend());
      }
    }
  }

  return creatureIds;
}

Tile* World::internalGetTile(const Position& position)
{
  if (position.getX() < position_offset ||
      position.getX() >= position_offset + worldSizeX_ ||
      position.getY() < position_offset ||
      position.getY() >= position_offset + worldSizeY_ ||
      position.getZ() != 7)
  {
    return nullptr;
  }

  const auto index = ((position.getX() - position_offset) * worldSizeY_) +
                      (position.getY() - position_offset);
  return &tiles_[index];
}

const Tile* World::getTile(const Position& position) const
{
  if (position.getX() < position_offset ||
      position.getX() >= position_offset + worldSizeX_ ||
      position.getY() < position_offset ||
      position.getY() >= position_offset + worldSizeY_ ||
      position.getZ() != 7)
  {
    return nullptr;
  }

  const auto index = ((position.getX() - position_offset) * worldSizeY_) +
                      (position.getY() - position_offset);
  return &tiles_[index];
}

Creature& World::internalGetCreature(CreatureId creatureId)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("%s: called with non-existent CreatureId: %d", __func__, creatureId);
  }
  return *(creature_data_.at(creatureId).creature);
}

const Creature& World::getCreature(CreatureId creatureId) const
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("%s: called with non-existent CreatureId: %d", __func__, creatureId);
    return Creature::INVALID;
  }
  return *(creature_data_.at(creatureId).creature);
}

CreatureCtrl& World::getCreatureCtrl(CreatureId creatureId)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("getCreatureCtrl called with non-existent CreatureId");
  }
  return *(creature_data_.at(creatureId).creature_ctrl);
}

const Position& World::getCreaturePosition(CreatureId creatureId) const
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("getCreaturePosition called with non-existent CreatureId");
    return Position::INVALID;
  }
  return creature_data_.at(creatureId).position;
}
