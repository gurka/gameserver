/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
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

#include "itemfactory.h"
#include "npcctrl.h"
#include "logger.h"
#include "tick.h"

World::World(std::unique_ptr<ItemFactory> itemFactory,
             int worldSizeX,
             int worldSizeY,
             std::unordered_map<Position, Tile, Position::Hash> tiles)
  : itemFactory_(std::move(itemFactory)),
    worldSizeX_(worldSizeX),
    worldSizeY_(worldSizeY),
    tiles_(std::move(tiles))
{
}

Position World::addCreature(Creature* creature, CreatureCtrl* creatureCtrl, const Position& position)
{
  auto creatureId = creature->getCreatureId();

  if (creatureExists(creatureId))
  {
    LOG_ERROR("addCreature: Creature already exists: %s (%d)",
                creature->getName().c_str(),
                creature->getCreatureId());
    return Position::INVALID;
  }

  if (!positionIsValid(position))
  {
    LOG_ERROR("addCreature: Invalid position: %s",
                position.toString().c_str());
    return Position::INVALID;
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
  auto found = false;
  for (const auto& offsets : positionOffsets)
  {
    adjustedPosition = Position(position.getX() + std::get<0>(offsets),
                                position.getY() + std::get<1>(offsets),
                                position.getZ());

    // TODO(gurka): Need to check more stuff (blocking, etc)
    if (internalGetTile(adjustedPosition).getCreatureIds().size() == 0)
    {
      found = true;
      break;
    }
  }

  if (found)
  {
    LOG_INFO("%s: Spawning Creature: %d at Position: %s", __func__, creatureId, adjustedPosition.toString().c_str());
    internalGetTile(adjustedPosition).addCreature(creatureId);

    creatures_.insert(std::make_pair(creatureId, creature));
    creatureCtrls_.insert(std::make_pair(creatureId, creatureCtrl));
    creaturePositions_.insert(std::make_pair(creatureId, adjustedPosition));

    // Tell near creatures that a creature has spawned
    // Except the spawned creature itself
    auto nearCreatureIds = getVisibleCreatureIds(adjustedPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      if (nearCreatureId != creatureId)
      {
        getCreatureCtrl(nearCreatureId).onCreatureSpawn(*creature, adjustedPosition);
      }
    }

    return adjustedPosition;
  }
  else
  {
    LOG_DEBUG("%s: Could not find a Tile to spawn Creature: %d", __func__, creatureId);
    return Position::INVALID;
  }
}

void World::removeCreature(CreatureId creatureId)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("removeCreature called with non-existent CreatureId");
    return;
  }

  const auto& creature = getCreature(creatureId);
  const auto& position = getCreaturePosition(creatureId);
  auto& tile = internalGetTile(position);
  auto stackPos = tile.getCreatureStackPos(creatureId);

  // Tell near creatures that a creature has despawned
  // Including the despawning creature!
  auto nearCreatureIds = getVisibleCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onCreatureDespawn(creature, position, stackPos);
  }

  creatures_.erase(creatureId);
  creatureCtrls_.erase(creatureId);
  creaturePositions_.erase(creatureId);
  tile.removeCreature(creatureId);
}

bool World::creatureExists(CreatureId creatureId) const
{
  return creatureId != Creature::INVALID_ID && creatures_.count(creatureId) == 1;
}

World::ReturnCode World::creatureMove(CreatureId creatureId, Direction direction)
{
  return creatureMove(creatureId, getCreaturePosition(creatureId).addDirection(direction));
}

World::ReturnCode World::creatureMove(CreatureId creatureId, const Position& toPosition)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("creatureMove called with non-existent CreatureId");
    return ReturnCode::INVALID_CREATURE;
  }

  if (!positionIsValid(toPosition))
  {
    LOG_ERROR("moveCreature: Invalid position: %s",
              toPosition.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Check if toTile is blocking or not
  auto& toTile = internalGetTile(toPosition);
  for (const auto& item : toTile.getItems())
  {
    if (item.isBlocking())
    {
      LOG_DEBUG("%s: Item on toTile is blocking", __func__);
      return ReturnCode::THERE_IS_NO_ROOM;
    }
  }
  // TODO(gurka): Creatures are also blocking!
  // (enforced by the client when walking with arrow keys)

  // Get Creature
  auto& creature = internalGetCreature(creatureId);

  // Check if Creature may move at this time
  auto current_tick = Tick::now();
  if (creature.getNextWalkTick() > current_tick)
  {
    LOG_DEBUG("%s: current_tick = %u nextWalkTick = %u => MAY_NOT_MOVE_YET",
              __func__,
              current_tick,
              creature.getNextWalkTick());
    return ReturnCode::MAY_NOT_MOVE_YET;
  }

  // Move the actual creature
  auto fromPosition = getCreaturePosition(creatureId);  // Need to create a new Position here (i.e. not auto&)
  auto& fromTile = internalGetTile(fromPosition);
  auto fromStackPos = fromTile.getCreatureStackPos(creatureId);
  fromTile.removeCreature(creatureId);

  toTile.addCreature(creatureId);
  auto toStackPos = toTile.getCreatureStackPos(creatureId);
  creaturePositions_.at(creatureId) = toPosition;

  // Set new nextWalkTime for this Creature
  auto groundSpeed = fromTile.getGroundSpeed();
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
  auto x_min = std::min(fromPosition.getX(), toPosition.getX());
  auto x_max = std::min(fromPosition.getX(), toPosition.getX());
  auto y_min = std::min(fromPosition.getY(), toPosition.getY());
  auto y_max = std::max(fromPosition.getY(), toPosition.getY());
  for (auto x = x_min - 9; x <= x_max + 9; x++)
  {
    for (auto y = y_min - 7; y <= y_max + 7; y++)
    {
      const auto& tile = getTile(Position(x, y, 7));

      const auto& nearCreatureIds = tile.getCreatureIds();
      for (const auto nearCreatureId : nearCreatureIds)
      {
        getCreatureCtrl(nearCreatureId).onCreatureMove(creature, fromPosition, fromStackPos, toPosition, toStackPos);
      }
    }
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the fromTile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (fromTile.getNumberOfThings() >= 10)
  {
    auto nearCreatureIds = getVisibleCreatureIds(fromPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onTileUpdate(fromPosition);
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
  auto stackPos = getTile(position).getCreatureStackPos(creatureId);
  auto nearCreatureIds = getVisibleCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onCreatureTurn(creature, position, stackPos);
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
  auto nearCreatureIds = getVisibleCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onCreatureSay(creature, position, message);
  }
}

World::ReturnCode World::addItem(ItemId itemId, const Position& position)
{
  auto item = itemFactory_->createItem(itemId);
  return addItem(item, position);
}

World::ReturnCode World::addItem(const Item& item, const Position& position)
{
  if (!positionIsValid(position))
  {
    LOG_ERROR("moveItem(): Invalid position: %s", position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Add Item to toTile
  auto& toTile = internalGetTile(position);
  toTile.addItem(item);

  // Call onItemAdded on all creatures that can see position
  auto nearCreatureIds = getVisibleCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemAdded(item, position);
  }

  return ReturnCode::OK;
}

World::ReturnCode World::removeItem(int itemId, int count, const Position& position, int stackPos)
{
  if (!positionIsValid(position))
  {
    LOG_ERROR("moveItem(): Invalid position: %s", position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Try to remove Item from fromTile
  auto& fromTile = internalGetTile(position);
  if (!fromTile.removeItem(itemId, stackPos))
  {
    LOG_ERROR("moveItem(): Could not remove item %d from %s", itemId, position.toString().c_str());
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Call onItemRemoved on all creatures that can see fromPosition
  auto nearCreatureIds = getVisibleCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemRemoved(position, stackPos);
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the fromTile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (fromTile.getNumberOfThings() >= 10)
  {
    auto nearCreatureIds = getVisibleCreatureIds(position);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onTileUpdate(position);
    }
  }

  return ReturnCode::OK;
}

World::ReturnCode World::moveItem(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                                  int itemId, int count, const Position& toPosition)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("moveItem(): called with non-existent CreatureId");
    return ReturnCode::INVALID_CREATURE;
  }

  if (!positionIsValid(fromPosition))
  {
    LOG_ERROR("moveItem(): Invalid fromPosition: %s",
                fromPosition.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  if (!positionIsValid(toPosition))
  {
    LOG_ERROR("moveItem(): Invalid toPosition: %s",
                toPosition.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  // Only allow move if the player is standing at or 1 sqm near the item
  const auto& position = getCreaturePosition(creatureId);
  if (std::abs(position.getX() - fromPosition.getX()) > 1 ||
      std::abs(position.getY() - fromPosition.getY()) > 1 ||
      position.getZ() != fromPosition.getZ())
  {
    LOG_DEBUG("moveItem(): Player is too far away");
    return ReturnCode::CANNOT_REACH_THAT_OBJECT;
  }

  if (itemId == 99)  // Item moved is a Creature
  {
    // We need to get the Creature that is being moved
    // It's not neccessarily the Creature that is doing the move

    // Check that count is correct (may only be 1)
    if (count != 1)
    {
      LOG_ERROR("%s: Trying to move a Creature, but count is not 1", __func__);
      return ReturnCode::ITEM_NOT_FOUND;
    }

    // Check if trying to move more than 1 tile
    if (std::abs(fromPosition.getX() - toPosition.getX()) > 1 ||
        std::abs(fromPosition.getY() - toPosition.getY()) > 1)
    {
      LOG_ERROR("%s: Trying to move a Creature more than 1 tile", __func__);
      return ReturnCode::OTHER_ERROR;
    }

    auto& fromTile = internalGetTile(fromPosition);
    auto movedCreatureId = fromTile.getCreatureId(fromStackPos);
    if (movedCreatureId == Creature::INVALID_ID)
    {
      LOG_ERROR("%s: Trying to move a Creature, but there is no Creature at the given position", __func__);
      return ReturnCode::ITEM_NOT_FOUND;
    }

    // Move the Creature as a regular Creature move
    return creatureMove(movedCreatureId, toPosition);
  }
  else  // Item moved is a regular Item
  {
    // Get tiles
    auto& fromTile = internalGetTile(fromPosition);
    auto& toTile = internalGetTile(toPosition);

    // Check if Item exists in fromTile
    auto item = fromTile.getItem(fromStackPos);
    if (!item.isValid())
    {
      LOG_DEBUG("%s: Could not find item %d at %s", __func__, itemId, fromPosition.toString().c_str());
      return ReturnCode::ITEM_NOT_FOUND;
    }

    // Check if we can add Item to toTile
    for (const auto& item : toTile.getItems())
    {
      if (item.isBlocking())
      {
        LOG_DEBUG("%s: Item on toTile is blocking", __func__);
        return ReturnCode::THERE_IS_NO_ROOM;
      }
    }

    // Try to remove Item from fromTile
    if (!fromTile.removeItem(itemId, fromStackPos))
    {
      LOG_DEBUG("%s: Could not remove item %d from %s", __func__, itemId, fromPosition.toString().c_str());
      return ReturnCode::ITEM_NOT_FOUND;
    }

    // Add Item to toTile
    toTile.addItem(item);

    // Call onItemRemoved on all creatures that can see fromPosition
    auto nearCreatureIds = getVisibleCreatureIds(fromPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onItemRemoved(fromPosition, fromStackPos);
    }

    // Call onItemAdded on all creatures that can see toPosition
    nearCreatureIds = getVisibleCreatureIds(toPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onItemAdded(item, toPosition);
    }

    // The client can only show ground + 9 Items/Creatures, so if the number of things on the fromTile
    // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
    if (fromTile.getNumberOfThings() >= 10)
    {
      auto nearCreatureIds = getVisibleCreatureIds(fromPosition);
      for (const auto& nearCreatureId : nearCreatureIds)
      {
        getCreatureCtrl(nearCreatureId).onTileUpdate(fromPosition);
      }
    }

    return ReturnCode::OK;
  }
}

bool World::creatureCanThrowTo(CreatureId creatureId, const Position& position) const
{
  return true;  // TODO(gurka): Fix
}

bool World::creatureCanReach(CreatureId creatureId, const Position& position) const
{
  const auto& creaturePosition = getCreaturePosition(creatureId);
  return !(std::abs(creaturePosition.getX() - position.getX()) > 1 ||
           std::abs(creaturePosition.getY() - position.getY()) > 1 ||
           creaturePosition.getZ() != position.getZ());
}

const std::vector<const Tile*> World::getMapBlock(const Position& position, int width, int height) const
{
  std::vector<const Tile*> tiles;
  for (auto x = 0; x < width; x++)
  {
    for (auto y = 0; y < height; y++)
    {
      Position temp(position.getX() + x, position.getY() + y, position.getZ());
      tiles.push_back(&getTile(temp));
    }
  }
  return tiles;
}

bool World::positionIsValid(const Position& position) const
{
  return position.getX() >= worldSizeStart_ &&
         position.getX() < worldSizeStart_ + worldSizeX_ &&
         position.getY() >= worldSizeStart_ &&
         position.getY() < worldSizeStart_ + worldSizeY_ &&
         position.getZ() == 7;
}

std::vector<CreatureId> World::getVisibleCreatureIds(const Position& position) const
{
  std::vector<CreatureId> creatureIds;

  for (int x = position.getX() - 9; x <= position.getX() + 9; ++x)
  {
    for (int y = position.getY() - 7; y <= position.getY() + 7; ++y)
    {
      Position tempPosition(x, y, position.getZ());

      if (!positionIsValid(tempPosition))
      {
        continue;
      }

      const Tile& tile = getTile(tempPosition);
      creatureIds.insert(creatureIds.cend(), tile.getCreatureIds().cbegin(), tile.getCreatureIds().cend());
    }
  }

  return creatureIds;
}

Tile& World::internalGetTile(const Position& position)
{
  if (!positionIsValid(position))
  {
    LOG_ERROR("getTile called with invalid Position");
  }
  return tiles_.at(position);
}

const Tile& World::getTile(const Position& position) const
{
  if (!positionIsValid(position))
  {
    LOG_ERROR("getTile called with invalid Position");
  }
  return tiles_.at(position);
}

Creature& World::internalGetCreature(CreatureId creatureId)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("getCreature called with non-existent CreatureId");
  }
  return *(creatures_.at(creatureId));
}

const Creature& World::getCreature(CreatureId creatureId) const
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("getCreature called with non-existent CreatureId");
    return Creature::INVALID;
  }
  return *(creatures_.at(creatureId));
}

CreatureCtrl& World::getCreatureCtrl(CreatureId creatureId)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("getCreatureCtrl called with non-existent CreatureId");
  }
  return *(creatureCtrls_.at(creatureId));
}

const Position& World::getCreaturePosition(CreatureId creatureId) const
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("getCreaturePosition called with non-existent CreatureId");
    return Position::INVALID;
  }
  return creaturePositions_.at(creatureId);
}
