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

#include "playerctrl.h"

#include <algorithm>
#include <deque>
#include <list>

#include "logger.h"
#include "world/position.h"
#include "world/tile.h"
#include "network/outgoingpacket.h"

void PlayerCtrl::onCreatureSpawn(const Creature& creature, const Position& position)
{
  OutgoingPacket packet;

  packet.addU8(0x6A);
  addPosition(position, &packet);
  addCreature(creature, &packet);

  // Login bubble
  packet.addU8(0x83);
  addPosition(position, &packet);
  packet.addU8(0x0A);

  sendPacket_(packet);
}

void PlayerCtrl::onCreatureDespawn(const Creature& creature, const Position& position, uint8_t stackPos)
{
  OutgoingPacket packet;

  // Logout poff
  packet.addU8(0x83);
  addPosition(position, &packet);
  packet.addU8(0x02);

  packet.addU8(0x6C);
  addPosition(position, &packet);
  packet.addU8(stackPos);

  sendPacket_(packet);
}

void PlayerCtrl::onCreatureMove(const Creature& creature,
                                const Position& oldPosition, uint8_t oldStackPos,
                                const Position& newPosition, uint8_t newStackPos)
{
  OutgoingPacket packet;

  bool canSeeOldPos = canSee(oldPosition);
  bool canSeeNewPos = canSee(newPosition);

  if (canSeeOldPos && canSeeNewPos)
  {
    packet.addU8(0x6D);
    packet.addU16(oldPosition.getX());
    packet.addU16(oldPosition.getY());
    packet.addU8(oldPosition.getZ());
    packet.addU8(oldStackPos);
    packet.addU16(newPosition.getX());
    packet.addU16(newPosition.getY());
    packet.addU8(newPosition.getZ());
  }
  else if (canSeeOldPos)
  {
    packet.addU8(0x6C);
    packet.addU16(oldPosition.getX());
    packet.addU16(oldPosition.getY());
    packet.addU8(oldPosition.getZ());
    packet.addU8(oldStackPos);
  }
  else if (canSeeNewPos)
  {
    packet.addU8(0x6A);
    packet.addU16(oldPosition.getX());
    packet.addU16(oldPosition.getY());
    packet.addU8(oldPosition.getZ());
    addCreature(creature, &packet);
  }

  if (creature.getCreatureId() == creatureId_)
  {
    // This player moved, send new map data
    if (oldPosition.getY() > newPosition.getY())
    {
      // Get north block
      packet.addU8(0x65);
      addMapData(Position(oldPosition.getX() - 8, newPosition.getY() - 6, 7), 18, 1, &packet);
      packet.addU8(0x7E);
      packet.addU8(0xFF);
    }
    else if (oldPosition.getY() < newPosition.getY())
    {
      // Get south block
      packet.addU8(0x67);
      addMapData(Position(oldPosition.getX() - 8, newPosition.getY() + 7, 7), 18, 1, &packet);
      packet.addU8(0x7E);
      packet.addU8(0xFF);
    }

    if (oldPosition.getX() > newPosition.getX())
    {
      // Get west block
      packet.addU8(0x68);
      addMapData(Position(newPosition.getX() - 8, newPosition.getY() - 6, 7), 1, 14, &packet);
      packet.addU8(0x62);
      packet.addU8(0xFF);
    }
    else if (oldPosition.getX() < newPosition.getX())
    {
      // Get west block
      packet.addU8(0x66);
      addMapData(Position(newPosition.getX() + 9, newPosition.getY() - 6, 7), 1, 14, &packet);
      packet.addU8(0x62);
      packet.addU8(0xFF);
    }
  }

  sendPacket_(packet);
}

void PlayerCtrl::onCreatureTurn(const Creature& creature, const Position& position, uint8_t stackPos)
{
  OutgoingPacket packet;

  packet.addU8(0x6B);
  addPosition(position, &packet);
  packet.addU8(stackPos);

  packet.addU8(0x63);
  packet.addU8(0x00);
  packet.addU32(creature.getCreatureId());
  packet.addU8(creature.getDirection());

  sendPacket_(packet);
}

void PlayerCtrl::onCreatureSay(const Creature& creature, const Position& position, const std::string& message)
{
  OutgoingPacket packet;

  packet.addU8(0xAA);
  packet.addString(creature.getName());
  packet.addU8(0x01);  // Say type

  // if type <= 3
  addPosition(position, &packet);

  packet.addString(message);

  sendPacket_(packet);
}

void PlayerCtrl::onItemRemoved(const Position& position, uint8_t stackPos)
{
  OutgoingPacket packet;

  packet.addU8(0x6C);
  addPosition(position, &packet);
  packet.addU8(stackPos);

  sendPacket_(packet);
}

void PlayerCtrl::onItemAdded(const Item& item, const Position& position)
{
  OutgoingPacket packet;

  packet.addU8(0x6A);
  addPosition(position, &packet);
  addItem(item, &packet);

  sendPacket_(packet);
}

void PlayerCtrl::onTileUpdate(const Position& position)
{
  OutgoingPacket packet;

  packet.addU8(0x69);
  addPosition(position, &packet);
  addMapData(position, 1, 1, &packet);
  packet.addU8(0x00);
  packet.addU8(0xFF);

  sendPacket_(packet);
}

void PlayerCtrl::onPlayerSpawn(const Player& player, const Position& position, const std::string& loginMessage)
{
  OutgoingPacket packet;

  packet.addU8(0x0A);  // Login
  packet.addU32(creatureId_);

  packet.addU8(0x32);  // ??
  packet.addU8(0x00);

  packet.addU8(0x64);  // Full (near) map
  packet.addU16(position.getX());  // Position
  packet.addU16(position.getY());
  packet.addU8(position.getZ());

  addMapData(Position(position.getX() - 8, position.getY() - 6, position.getZ()), 18, 14, &packet);

  for (auto i = 0; i < 12; i++)
  {
    packet.addU8(0xFF);
  }

  packet.addU8(0xE4);  // Light?
  packet.addU8(0xFF);

  packet.addU8(0x83);  // Magic effect (login)
  packet.addU16(position.getX());
  packet.addU16(position.getY());
  packet.addU8(position.getZ());
  packet.addU8(0x0A);

  // Player stats
  packet.addU8(0xA0);
  packet.addU16(player.getHealth());
  packet.addU16(player.getMaxHealth());
  packet.addU16(player.getCapacity());
  packet.addU32(player.getExperience());
  packet.addU8(player.getLevel());
  packet.addU16(player.getMana());
  packet.addU16(player.getMaxMana());
  packet.addU8(player.getMagicLevel());

  packet.addU8(0x82);  // Light?
  packet.addU8(0x6F);
  packet.addU8(0xD7);

  // Player skills
  packet.addU8(0xA1);
  for (auto i = 0; i < 7; i++)
  {
    packet.addU8(10);
  }


  for (auto i = 1; i <= 10; i++)
  {
    addEquipment(player, i, &packet);
  }

  // Login message
  packet.addU8(0xB4);  // Message
  packet.addU8(0x11);  // Message type
  packet.addString(loginMessage);  // Message text

  sendPacket_(packet);
}

void PlayerCtrl::onEquipmentUpdated(const Player& player, int inventoryIndex)
{
  OutgoingPacket packet;

  addEquipment(player, inventoryIndex, &packet);

  sendPacket_(packet);
}

void PlayerCtrl::onUseItem(const Item& item)
{
  if (!item.hasAttribute("maxitems"))
  {
    LOG_ERROR("onUseItem(): Container Item: %d missing \"maxitems\" attribute", item.getItemId());
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0x6E);
  packet.addU8(0x00);  // Level / Depth

  packet.addU16(item.getItemId());  // Container ID
  packet.addString(item.getName());
  packet.addU16(item.getAttribute<int>("maxitems"));

  packet.addU8(0x00);  // Number of items

  sendPacket_(packet);
}

void PlayerCtrl::sendTextMessage(const std::string& message)
{
  OutgoingPacket packet;

  packet.addU8(0xB4);
  packet.addU8(0x13);
  packet.addString(message);

  sendPacket_(packet);
}

void PlayerCtrl::sendCancel(const std::string& message)
{
  OutgoingPacket packet;

  packet.addU8(0xB4);
  packet.addU8(0x14);
  packet.addString(message);

  sendPacket_(packet);
}

bool PlayerCtrl::canSee(const Position& position) const
{
  const Position& playerPosition = worldInterface_->getCreaturePosition(creatureId_);
  return position.getX() >= playerPosition.getX() - 8 &&
         position.getX() < playerPosition.getX() + 8 &&
         position.getY() >= playerPosition.getY() - 6 &&
         position.getY() < playerPosition.getY() + 6;
}

void PlayerCtrl::addPosition(const Position& position, OutgoingPacket* packet) const
{
  packet->addU16(position.getX());
  packet->addU16(position.getY());
  packet->addU8(position.getZ());
}

void PlayerCtrl::addMapData(const Position& position, int width, int height, OutgoingPacket* packet)
{
  std::list<const Tile*> tiles = worldInterface_->getMapBlock(position, width, height);
  std::list<const Tile*>::const_iterator it = tiles.begin();

  for (auto x = 0; x < width; x++)
  {
    for (auto y = 0; y < height; y++)
    {
      const Tile* tile = *it;
      if (tile != nullptr)
      {
        // Client can only handle ground + 9 items/creatures at most
        auto count = 0;

        packet->addU16(tile->getGroundItem().getItemId());
        count++;

        // if splash; add; count++

        const std::deque<Item>& topItems = tile->getTopItems();
        for (auto it = topItems.cbegin(); it != topItems.cend() && count < 10; ++it)
        {
          addItem(*it, packet);
          count++;
        }

        const std::deque<CreatureId>& creatureIds = tile->getCreatureIds();
        for (auto it = creatureIds.cbegin(); it != creatureIds.cend() && count < 10; ++it)
        {
          const Creature& creature = worldInterface_->getCreature(*it);
          addCreature(creature, packet);
          count++;
        }

        const std::deque<Item>& bottomItems = tile->getBottomItems();
        for (auto it = bottomItems.cbegin(); it != bottomItems.cend() && count < 10; ++it)
        {
          addItem(*it, packet);
          count++;
        }
      }

      if (x != width - 1 || y != height - 1)
      {
        packet->addU8(0x00);
        packet->addU8(0xFF);
      }

      ++it;
    }
  }
}

void PlayerCtrl::addCreature(const Creature& creature, OutgoingPacket* packet)
{
  // First check if we know about this creature or not
  if (knownCreatures_.count(creature.getCreatureId()) == 0)
  {
    // Nope, then add it
    knownCreatures_.insert(creature.getCreatureId());

    if (knownCreatures_.size() > 64)
    {
      // TODO(gurka): remove a known creature
    }

    packet->addU8(0x61);
    packet->addU8(0x00);
    packet->addU32(0x00);  // creatureId to remove (0x00 = none)
    packet->addU32(creature.getCreatureId());
    packet->addString(creature.getName());
  }
  else
  {
    // We already know about this creature
    packet->addU8(0x62);
    packet->addU8(0x00);
    packet->addU32(creature.getCreatureId());
  }

  packet->addU8(creature.getHealth() / creature.getMaxHealth() * 100);
  packet->addU8(static_cast<uint8_t>(creature.getDirection()));
  packet->addU8(creature.getOutfit().type);
  packet->addU8(creature.getOutfit().head);
  packet->addU8(creature.getOutfit().body);
  packet->addU8(creature.getOutfit().legs);
  packet->addU8(creature.getOutfit().feet);

  packet->addU8(0x00);
  packet->addU8(0xDC);

  packet->addU16(creature.getSpeed());
}

void PlayerCtrl::addItem(const Item& item, OutgoingPacket* packet) const
{
  packet->addU16(item.getItemId());
  if (item.isStackable())
  {
    packet->addU8(item.getCount());
  }
  else if (item.isMultitype())
  {
    packet->addU8(item.getSubtype());
  }
}

void PlayerCtrl::addEquipment(const Player& player, int inventoryIndex, OutgoingPacket* packet) const
{
  const auto& equipment = player.getEquipment();
  const auto& item = equipment.getItem(inventoryIndex);

  if (!item.isValid())
  {
    packet->addU8(0x79);  // No Item in this slot
    packet->addU8(inventoryIndex);
  }
  else
  {
    packet->addU8(0x78);
    packet->addU8(inventoryIndex);
    addItem(item, packet);
  }
}
