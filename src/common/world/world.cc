/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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
#include <deque>
#include <fstream>
#include <sstream>

#include "npcctrl.h"
#include "logger.h"
#include "rapidxml.hpp"

World::World(const std::string& dataFilename, const std::string& worldFilename, const std::string& itemsFilename)
  : dataFilename_(dataFilename),
    worldFilename_(worldFilename),
    itemsFilename_(itemsFilename)
{
}

bool World::initialize()
{
  // Load data file
  LOG_INFO("Loading data files");
  if (!ItemData::loadItemData(dataFilename_, itemsFilename_))
  {
    // ItemData::loadItemData logs error on failure
    return false;
  }

  // Open world.xml and read it into a string
  LOG_INFO("Loading world file: \"%s\"", worldFilename_.c_str());
  std::ifstream xmlFile(worldFilename_);
  if (!xmlFile.is_open())
  {
    LOG_ERROR("initialize(): Could not open file: \"%s\"", worldFilename_.c_str());
    return false;
  }

  std::string tempString;
  std::ostringstream xmlStringStream;
  while (std::getline(xmlFile, tempString))
  {
    xmlStringStream << tempString << "\n";
  }

  // Convert the std::string to a char*
  char* xmlString = strdup(xmlStringStream.str().c_str());

  // Parse the XML string with Rapidxml
  rapidxml::xml_document<> worldXml;
  worldXml.parse<0>(xmlString);

  // Get top node (<map>)
  auto* mapNode = worldXml.first_node();

  // Read width and height
  auto* widthAttr = mapNode->first_attribute("width");
  auto* heightAttr = mapNode->first_attribute("height");
  if (widthAttr == nullptr || heightAttr == nullptr)
  {
    LOG_ERROR("initialize(): Invalid file, missing attributes width or height in <map>-node");
    free(xmlString);
    return false;
  }

  worldSizeX_ = std::stoi(widthAttr->value());
  worldSizeY_ = std::stoi(heightAttr->value());

  auto* tileNode = mapNode->first_node();
  for (int y = worldSizeStart_; y < worldSizeStart_ + worldSizeY_; y++)
  {
    for (int x = worldSizeStart_; x < worldSizeStart_ + worldSizeX_; x++)
    {
      Position position(x, y, 7);

      if (tileNode == nullptr)
      {
        LOG_ERROR("initialize(): Invalid file, missing <tile>-node");
        free(xmlString);
        return false;
      }

      // Read the first <item> (there must be at least one, the ground item)
      auto* groundItemNode = tileNode->first_node();
      if (groundItemNode == nullptr)
      {
        LOG_ERROR("initialize(): Invalid file, <tile>-node is missing <item>-node");
        free(xmlString);
        return false;
      }
      auto* groundItemAttr = groundItemNode->first_attribute("id");
      if (groundItemAttr == nullptr)
      {
        LOG_ERROR("initialize(): Invalid file, missing attribute id in <item>-node");
        free(xmlString);
        return false;
      }

      auto groundItemId = std::stoi(groundItemAttr->value());
      tiles_.insert(std::make_pair(position, Tile(groundItemId)));

      // Read more items to put in this tile
      // But due to the way otserv-3.0 made world.xml, do it backwards
      for (auto* itemNode = tileNode->last_node(); itemNode != groundItemNode; itemNode = itemNode->previous_sibling())
      {
        auto* itemIdAttr = itemNode->first_attribute("id");
        if (itemIdAttr == nullptr)
        {
          LOG_ERROR("initialize(): Invalid file, missing attribute id in <item>-node");
          free(xmlString);
          return false;
        }

        auto itemId = std::stoi(itemIdAttr->value());
        getTile(position).addItem(Item(itemId));
      }

      // Go to next <tile> in XML
      tileNode = tileNode->next_sibling();
    }
  }

  LOG_INFO("World loaded, size: %d x %d", worldSizeX_, worldSizeY_);

  free(xmlString);
  return true;
}

void World::addCreature(Creature* creature, CreatureCtrl* creatureCtrl, const Position& position)
{
  auto creatureId = creature->getCreatureId();

  if (creatureExists(creatureId))
  {
    LOG_ERROR("addCreature: Creature already exists: %s (%d)",
                creature->getName().c_str(),
                creature->getCreatureId());
    return;
  }

  if (!positionIsValid(position))
  {
    LOG_ERROR("addCreature: Invalid position: %s",
                position.toString().c_str());
    return;
  }

  auto& tile = getTile(position);
  if (tile.getCreatureIds().size() != 0)
  {
    LOG_ERROR("addCreature: Already a creature at position: %s",
                position.toString().c_str());
    return;
  }
  tile.addCreature(creatureId);

  creatures_.insert(std::make_pair(creatureId, creature));
  creatureCtrls_.insert(std::make_pair(creatureId, creatureCtrl));
  creaturePositions_.insert(std::make_pair(creatureId, position));

  // Tell near creatures that a creature has spawned
  // Except the spawned creature itself
  std::list<CreatureId> nearCreatureIds = getNearCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    if (nearCreatureId != creatureId)
    {
      getCreatureCtrl(nearCreatureId).onCreatureSpawn(*creature, position);
    }
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
  auto& tile = getTile(position);
  auto stackPos = tile.getCreatureStackPos(creatureId);

  // Tell near creatures that a creature has despawned
  // Except the despawning creature
  std::list<CreatureId> nearCreatureIds = getNearCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    if (nearCreatureId != creatureId)
    {
      getCreatureCtrl(nearCreatureId).onCreatureDespawn(creature, position, stackPos);
    }
  }

  creatures_.erase(creatureId);
  creatureCtrls_.erase(creatureId);
  creaturePositions_.erase(creatureId);
  tile.removeCreature(creatureId);
}

void World::creatureMove(CreatureId creatureId, Direction direction)
{
  creatureMove(creatureId, getCreaturePosition(creatureId).addDirection(direction));
}

void World::creatureMove(CreatureId creatureId, const Position& newPosition)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("creatureMove called with non-existent CreatureId");
    return;
  }

  if (!positionIsValid(newPosition))
  {
    LOG_ERROR("moveCreature: Invalid position: %s",
                newPosition.toString().c_str());
    return;
  }

  auto& creature = getCreature(creatureId);

  // Move the actual creature
  auto oldPosition = getCreaturePosition(creatureId);
  auto oldStackPos = getTile(oldPosition).getCreatureStackPos(creatureId);
  getTile(oldPosition).removeCreature(creatureId);

  getTile(newPosition).addCreature(creatureId);
  auto newStackPos = getTile(newPosition).getCreatureStackPos(creatureId);
  creaturePositions_.at(creatureId) = newPosition;


  // Update direction
  if (oldPosition.getY() > newPosition.getY())
  {
    creature.setDirection(Direction::NORTH);
  }
  else if (oldPosition.getY() < newPosition.getY())
  {
    creature.setDirection(Direction::SOUTH);
  }
  if (oldPosition.getX() > newPosition.getX())
  {
    creature.setDirection(Direction::WEST);
  }
  else if (oldPosition.getX() < newPosition.getX())
  {
    creature.setDirection(Direction::EAST);
  }

  // Call onCreatureMove on all creatures that can see the movement
  // including the moving creature itself
  auto x_min = std::min(oldPosition.getX(), newPosition.getX());
  auto x_max = std::min(oldPosition.getX(), newPosition.getX());
  auto y_min = std::min(oldPosition.getY(), newPosition.getY());
  auto y_max = std::max(oldPosition.getY(), newPosition.getY());
  for (auto x = x_min - 9; x <= x_max + 9; x++)
  {
    for (auto y = y_min - 7; y <= y_max + 7; y++)
    {
      const Tile& tile = getTile(Position(x, y, 7));

      const std::deque<CreatureId>& nearCreatureIds = tile.getCreatureIds();
      for (const auto nearCreatureId : nearCreatureIds)
      {
        getCreatureCtrl(nearCreatureId).onCreatureMove(creature, oldPosition, oldStackPos, newPosition, newStackPos);
      }
    }
  }
}

void World::creatureTurn(CreatureId creatureId, Direction direction)
{
  if (!creatureExists(creatureId))
  {
    LOG_ERROR("creatureTurn called with non-existent CreatureId");
    return;
  }

  Creature& creature = getCreature(creatureId);
  creature.setDirection(direction);

  // Call onCreatureTurn on all creatures that can see the turn
  // including the turning creature itself
  Position position = getCreaturePosition(creatureId);
  auto stackPos = getTile(position).getCreatureStackPos(creatureId);
  std::list<CreatureId> nearCreatureIds = getNearCreatureIds(position);
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
  std::list<CreatureId> nearCreatureIds = getNearCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onCreatureSay(creature, position, message);
  }
}

World::ReturnCode World::addItem(int itemId, int count, const Position& position)
{
  if (!positionIsValid(position))
  {
    LOG_ERROR("moveItem(): Invalid position: %s", position.toString().c_str());
    return ReturnCode::INVALID_POSITION;
  }

  Item item(itemId);

  // Add Item to toTile
  auto& toTile = getTile(position);
  toTile.addItem(item);

  // Call onItemAdded on all creatures that can see position
  auto nearCreatureIds = getNearCreatureIds(position);
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

  Item item(itemId);

  // Try to remove Item from fromTile
  auto& fromTile = getTile(position);
  if (!fromTile.removeItem(item, stackPos))
  {
    LOG_ERROR("moveItem(): Could not remove item %d from %s", itemId, position.toString().c_str());
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Call onItemRemoved on all creatures that can see fromPosition
  auto nearCreatureIds = getNearCreatureIds(position);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemRemoved(position, stackPos);
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the fromTile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (fromTile.getNumberOfThings() >= 10)
  {
    std::list<CreatureId> nearCreatureIds = getNearCreatureIds(position);
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

  if (itemId == 99)
  {
    LOG_DEBUG("moveItem(): Item is a Creature");
    return ReturnCode::CANNOT_MOVE_THAT_OBJECT;
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

  Item item(itemId);

  // Try to remove Item from fromTile
  auto& fromTile = getTile(fromPosition);
  if (!fromTile.removeItem(item, fromStackPos))
  {
    LOG_ERROR("moveItem(): Could not remove item %d from %s", itemId, fromPosition.toString().c_str());
    return ReturnCode::ITEM_NOT_FOUND;
  }

  // Add Item to toTile
  auto& toTile = getTile(toPosition);
  toTile.addItem(item);

  // Call onItemRemoved on all creatures that can see fromPosition
  auto nearCreatureIds = getNearCreatureIds(fromPosition);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemRemoved(fromPosition, fromStackPos);
  }

  // Call onItemAdded on all creatures that can see toPosition
  nearCreatureIds = getNearCreatureIds(toPosition);
  for (const auto& nearCreatureId : nearCreatureIds)
  {
    getCreatureCtrl(nearCreatureId).onItemAdded(Item(itemId), toPosition);
  }

  // The client can only show ground + 9 Items/Creatures, so if the number of things on the fromTile
  // is >= 10 then some items on the tile is unknown to the client, so update the Tile for each nearby Creature
  if (fromTile.getNumberOfThings() >= 10)
  {
    std::list<CreatureId> nearCreatureIds = getNearCreatureIds(fromPosition);
    for (const auto& nearCreatureId : nearCreatureIds)
    {
      getCreatureCtrl(nearCreatureId).onTileUpdate(fromPosition);
    }
  }

  return ReturnCode::OK;
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

const std::list<const Tile*> World::getMapBlock(const Position& position, int width, int height) const
{
  std::list<const Tile*> tiles;
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

bool World::creatureExists(CreatureId creatureId) const
{
  return creatureId != Creature::INVALID_ID && creatures_.count(creatureId) == 1;
}

bool World::positionIsValid(const Position& position) const
{
  return position.getX() >= worldSizeStart_ &&
         position.getX() < worldSizeStart_ + worldSizeX_ &&
         position.getY() >= worldSizeStart_ &&
         position.getY() < worldSizeStart_ + worldSizeY_ &&
         position.getZ() == 7;
}

std::list<CreatureId> World::getNearCreatureIds(const Position& position) const
{
  std::list<CreatureId> creatureIds;

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

Tile& World::getTile(const Position& position)
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

Creature& World::getCreature(CreatureId creatureId)
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
  }
  return creaturePositions_.at(creatureId);
}
