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

#include "protocol_71.h"

#include <algorithm>
#include <deque>
#include <utility>

#include "gameengine.h"
#include "server.h"
#include "incomingpacket.h"
#include "outgoingpacket.h"
#include "worldinterface.h"
#include "logger.h"
#include "tile.h"
#include "account.h"

Protocol71::Protocol71(const std::function<void(void)>& closeProtocol,
                       GameEngine* gameEngine,
                       ConnectionId connectionId,
                       Server* server,
                       AccountReader* accountReader)
  : closeProtocol_(closeProtocol),
    playerId_(Creature::INVALID_ID),
    gameEngine_(gameEngine),
    connectionId_(connectionId),
    server_(server),
    accountReader_(accountReader)
{
  std::fill(knownCreatures_.begin(), knownCreatures_.end(), Creature::INVALID_ID);
}

void Protocol71::disconnected()
{
  // We may not send any more packets now
  server_ = nullptr;

  if (isLoggedIn())
  {
    // Tell gameengine to despawn us
    gameEngine_->despawn(playerId_);
  }
  else
  {
    // We are not logged in to the game, close the protocol now
    closeProtocol_();  // WARNING: This instance is deleted after this call
  }
}

void Protocol71::parsePacket(IncomingPacket* packet)
{
  if (!isConnected())
  {
    LOG_ERROR("%s: not connected", __func__);
    return;
  }

  if (!isLoggedIn())
  {
    // Not logged in, only allow login packet
    auto packetType = packet->getU8();
    if (packetType == 0x0A)
    {
      parseLogin(packet);
    }
    else
    {
      LOG_ERROR("%s: Expected login packet but received packet type: 0x%X", __func__, packetType);
      server_->closeConnection(connectionId_, true);
    }

    return;
  }

  while (!packet->isEmpty())
  {
    uint8_t packetId = packet->getU8();
    switch (packetId)
    {
      case 0x14:
      {
        gameEngine_->despawn(playerId_);
        break;
      }

      case 0x64:
      {
        parseMoveClick(packet);
        break;
      }

      case 0x65:  // Player move, North = 0
      case 0x66:  // East  = 1
      case 0x67:  // South = 2
      case 0x68:  // West  = 3
      {
        gameEngine_->move(playerId_, static_cast<Direction>(packetId - 0x65));
        break;
      }

      case 0x69:
      {
        gameEngine_->cancelMove(playerId_);
        break;
      }

      case 0x6F:  // Player turn, North = 0
      case 0x70:  // East  = 1
      case 0x71:  // South = 2
      case 0x72:  // West  = 3
      {
        gameEngine_->turn(playerId_, static_cast<Direction>(packetId - 0x6F));
        break;
      }

      case 0x78:
      {
        parseMoveItem(packet);
        break;
      }

      case 0x82:
      {
        parseUseItem(packet);
        break;
      }

      case 0x8C:
      {
        parseLookAt(packet);
        break;
      }

      case 0x96:
      {
        parseSay(packet);
        break;
      }

      case 0xBE:
      {
        // TODO(gurka): This packet more likely means "stop all actions", not only moving
        parseCancelMove(packet);
        break;
      }

      default:
      {
        LOG_ERROR("Unknown packet from player id: %d, packet id: 0x%X", playerId_, packetId);
        return;  // Don't read any more, even though there might be more packets that we can parse
      }
    }
  }
}

void Protocol71::onCreatureSpawn(const WorldInterface& world_interface, const Creature& creature, const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  if (creature.getCreatureId() == playerId_)
  {
    // We are spawning!
    const auto& player = static_cast<const Player&>(creature);

    packet.addU8(0x0A);  // Login
    packet.addU32(playerId_);

    packet.addU8(0x32);  // ??
    packet.addU8(0x00);

    packet.addU8(0x64);  // Full (visible) map
    addPosition(position, &packet);  // Position

    addMapData(world_interface, Position(position.getX() - 8, position.getY() - 6, position.getZ()), 18, 14, &packet);

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
  }
  else
  {
    // Someone else spawned
    packet.addU8(0x6A);
    addPosition(position, &packet);
    addCreature(creature, &packet);

    // Spawn/login bubble
    packet.addU8(0x83);
    addPosition(position, &packet);
    packet.addU8(0x0A);
  }

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onCreatureDespawn(const WorldInterface& world_interface, const Creature& creature, const Position& position, uint8_t stackPos)
{
  if (!isConnected())
  {
    if (creature.getCreatureId() == playerId_)
    {
      // We are no longer in game and the connection has been closed, close the protocol
      closeProtocol_();  // WARNING: This instance is deleted after this call
    }
    return;
  }

  OutgoingPacket packet;

  // Logout poff
  packet.addU8(0x83);
  addPosition(position, &packet);
  packet.addU8(0x02);

  packet.addU8(0x6C);
  addPosition(position, &packet);
  packet.addU8(stackPos);

  server_->sendPacket(connectionId_, std::move(packet));

  if (creature.getCreatureId() == playerId_)
  {
    // This player despawned!
    server_->closeConnection(connectionId_, false);
    closeProtocol_();  // WARNING: This instance is deleted after this call
  }
}

void Protocol71::onCreatureMove(const WorldInterface& world_interface,
                                const Creature& creature,
                                const Position& oldPosition,
                                uint8_t oldStackPos,
                                const Position& newPosition,
                                uint8_t newStackPos)
{
  if (!isConnected())
  {
    return;
  }

  // Build outgoing packet
  OutgoingPacket packet;

  const auto& player_position = world_interface.getCreaturePosition(playerId_);
  bool canSeeOldPos = canSee(player_position, oldPosition);
  bool canSeeNewPos = canSee(player_position, newPosition);

  if (canSeeOldPos && canSeeNewPos)
  {
    packet.addU8(0x6D);
    addPosition(oldPosition, &packet);
    packet.addU8(oldStackPos);
    addPosition(newPosition, &packet);
  }
  else if (canSeeOldPos)
  {
    packet.addU8(0x6C);
    addPosition(oldPosition, &packet);
    packet.addU8(oldStackPos);
  }
  else if (canSeeNewPos)
  {
    packet.addU8(0x6A);
    addPosition(newPosition, &packet);
    addCreature(creature, &packet);
  }

  if (creature.getCreatureId() == playerId_)
  {
    // This player moved, send new map data
    if (oldPosition.getY() > newPosition.getY())
    {
      // Get north block
      packet.addU8(0x65);
      addMapData(world_interface, Position(oldPosition.getX() - 8, newPosition.getY() - 6, 7), 18, 1, &packet);
      packet.addU8(0x7E);
      packet.addU8(0xFF);
    }
    else if (oldPosition.getY() < newPosition.getY())
    {
      // Get south block
      packet.addU8(0x67);
      addMapData(world_interface, Position(oldPosition.getX() - 8, newPosition.getY() + 7, 7), 18, 1, &packet);
      packet.addU8(0x7E);
      packet.addU8(0xFF);
    }

    if (oldPosition.getX() > newPosition.getX())
    {
      // Get west block
      packet.addU8(0x68);
      addMapData(world_interface, Position(newPosition.getX() - 8, newPosition.getY() - 6, 7), 1, 14, &packet);
      packet.addU8(0x62);
      packet.addU8(0xFF);
    }
    else if (oldPosition.getX() < newPosition.getX())
    {
      // Get west block
      packet.addU8(0x66);
      addMapData(world_interface, Position(newPosition.getX() + 9, newPosition.getY() - 6, 7), 1, 14, &packet);
      packet.addU8(0x62);
      packet.addU8(0xFF);
    }
  }

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onCreatureTurn(const WorldInterface& world_interface, const Creature& creature, const Position& position, uint8_t stackPos)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0x6B);
  addPosition(position, &packet);
  packet.addU8(stackPos);

  packet.addU8(0x63);
  packet.addU8(0x00);
  packet.addU32(creature.getCreatureId());
  packet.addU8(static_cast<uint8_t>(creature.getDirection()));

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onCreatureSay(const WorldInterface& world_interface, const Creature& creature, const Position& position, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0xAA);
  packet.addString(creature.getName());
  packet.addU8(0x01);  // Say type

  // if type <= 3
  addPosition(position, &packet);

  packet.addString(message);

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onItemRemoved(const WorldInterface& world_interface, const Position& position, uint8_t stackPos)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0x6C);
  addPosition(position, &packet);
  packet.addU8(stackPos);

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onItemAdded(const WorldInterface& world_interface, const Item& item, const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0x6A);
  addPosition(position, &packet);
  addItem(item, &packet);

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onTileUpdate(const WorldInterface& world_interface, const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0x69);
  addPosition(position, &packet);
  addMapData(world_interface, position, 1, 1, &packet);
  packet.addU8(0x00);
  packet.addU8(0xFF);

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onEquipmentUpdated(const Player& player, int inventoryIndex)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  addEquipment(player, inventoryIndex, &packet);

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onUseItem(const Item& item)
{
  if (!isConnected())
  {
    return;
  }

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

  server_->sendPacket(connectionId_, std::move(packet));
}

// 0x13 default text, 0x11 login text
void Protocol71::sendTextMessage(uint8_t message_type, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0xB4);
  packet.addU8(message_type);
  packet.addString(message);

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::sendCancel(const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  packet.addU8(0xB4);
  packet.addU8(0x14);
  packet.addString(message);

  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::cancelMove()
{
  OutgoingPacket packet;
  packet.addU8(0xB5);
  server_->sendPacket(connectionId_, std::move(packet));
}

bool Protocol71::canSee(const Position& from_position, const Position& to_position) const
{
  return to_position.getX() >  from_position.getX() - 9 &&
         to_position.getX() <= from_position.getX() + 9 &&
         to_position.getY() >  from_position.getY() - 7 &&
         to_position.getY() <= from_position.getY() + 7;
}

void Protocol71::addPosition(const Position& position, OutgoingPacket* packet) const
{
  packet->addU16(position.getX());
  packet->addU16(position.getY());
  packet->addU8(position.getZ());
}

void Protocol71::addMapData(const WorldInterface& world_interface, const Position& position, int width, int height, OutgoingPacket* packet)
{
  auto tiles = world_interface.getMapBlock(position, width, height);
  decltype(tiles)::const_iterator it = tiles.begin();

  for (auto x = 0; x < width; x++)
  {
    for (auto y = 0; y < height; y++)
    {
      const auto* tile = *it;
      if (tile != nullptr)
      {
        const auto& items = tile->getItems();
        const auto& creatureIds = tile->getCreatureIds();
        auto itemIt = items.cbegin();
        auto creatureIt = creatureIds.cbegin();

        // Client can only handle ground + 9 items/creatures at most
        auto count = 0;

        // Add ground Item
        addItem(*itemIt, packet);
        count++;
        ++itemIt;

        // if splash; add; count++

        // Add top Items
        while (count < 10 && itemIt != items.cend())
        {
          if (!itemIt->alwaysOnTop())
          {
            break;
          }

          addItem(*itemIt, packet);
          count++;
          ++itemIt;
        }

        // Add Creatures
        while (count < 10 && creatureIt != creatureIds.cend())
        {
          const Creature& creature = world_interface.getCreature(*creatureIt);
          addCreature(creature, packet);
          count++;
          ++creatureIt;
        }

        // Add bottom Item
        while (count < 10 && itemIt != items.cend())
        {
          addItem(*itemIt, packet);
          count++;
          ++itemIt;
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

void Protocol71::addCreature(const Creature& creature, OutgoingPacket* packet)
{
  // First check if we know about this creature or not
  auto it = std::find(knownCreatures_.begin(), knownCreatures_.end(), creature.getCreatureId());
  if (it == knownCreatures_.end())
  {
    // Find an empty spot
    auto unused = std::find(knownCreatures_.begin(), knownCreatures_.end(), Creature::INVALID_ID);
    if (unused == knownCreatures_.end())
    {
      // No empty spot!
      // TODO(gurka): Figure out how to handle this
      LOG_ERROR("%s: knownCreatures_ is full!");
    }
    else
    {
      *unused = creature.getCreatureId();
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

void Protocol71::addItem(const Item& item, OutgoingPacket* packet) const
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

void Protocol71::addEquipment(const Player& player, int inventoryIndex, OutgoingPacket* packet) const
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

void Protocol71::parseLogin(IncomingPacket* packet)
{
  packet->getU8();  // Unknown (0x02)
  uint8_t client_os = packet->getU8();
  uint16_t client_version = packet->getU16();
  packet->getU8();  // Unknown
  std::string character_name = packet->getString();
  std::string password = packet->getString();

  LOG_DEBUG("Client OS: %d Client version: %d Character: %s Password: %s",
            client_os,
            client_version,
            character_name.c_str(),
            password.c_str());

  // Check if character exists
  if (!accountReader_->characterExists(character_name))
  {
    OutgoingPacket response;
    response.addU8(0x14);
    response.addString("Invalid character.");
    server_->sendPacket(connectionId_, std::move(response));
    server_->closeConnection(connectionId_, false);
    return;
  }
  // Check if password is correct
  else if (!accountReader_->verifyPassword(character_name, password))
  {
    OutgoingPacket response;
    response.addU8(0x14);
    response.addString("Invalid password.");
    server_->sendPacket(connectionId_, std::move(response));
    server_->closeConnection(connectionId_, false);
    return;
  }

  // Login OK, add Player to GameEngine
  gameEngine_->spawn(character_name, this);
}

void Protocol71::parseMoveClick(IncomingPacket* packet)
{
  std::deque<Direction> moves;
  uint8_t pathLength = packet->getU8();

  if (pathLength == 0)
  {
    LOG_ERROR("%s: Path length is zero!", __func__);
    return;
  }

  for (auto i = 0; i < pathLength; i++)
  {
    moves.push_back(static_cast<Direction>(packet->getU8()));
  }

  gameEngine_->movePath(playerId_, moves);
}

void Protocol71::parseMoveItem(IncomingPacket* packet)
{
  // There are four options here:
  // Moving from inventory to inventory
  // Moving from inventory to Tile
  // Moving from Tile to inventory
  // Moving from Tile to Tile
  if (packet->peekU16() == 0xFFFF)
  {
    // Moving from inventory ...
    packet->getU16();

    auto fromInventoryId = packet->getU8();
    auto unknown = packet->getU16();
    auto itemId = packet->getU16();
    auto unknown2 = packet->getU8();

    if (packet->peekU16() == 0xFFFF)
    {
      // ... to inventory
      packet->getU16();
      auto toInventoryId = packet->getU8();
      auto unknown3 = packet->getU16();
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("%s: Moving %u (countOrSubType %u) from inventoryId %u to iventoryId %u (unknown: %u, unknown2: %u, unknown3: %u)",
                __func__,
                itemId,
                countOrSubType,
                fromInventoryId,
                toInventoryId,
                unknown,
                unknown2,
                unknown3);

      gameEngine_->moveItemFromInvToInv(playerId_, fromInventoryId, itemId, countOrSubType, toInventoryId);
    }
    else
    {
      // ... to Tile
      auto toPosition = getPosition(packet);
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("%s: Moving %u (countOrSubType %u) from inventoryId %u to %s (unknown: %u, unknown2: %u)",
                __func__,
                itemId,
                countOrSubType,
                fromInventoryId,
                toPosition.toString().c_str(),
                unknown,
                unknown2);

      gameEngine_->moveItemFromInvToPos(playerId_, fromInventoryId, itemId, countOrSubType, toPosition);
    }
  }
  else
  {
    // Moving from Tile ...
    auto fromPosition = getPosition(packet);
    auto itemId = packet->getU16();
    auto fromStackPos = packet->getU8();

    if (packet->peekU16() == 0xFFFF)
    {
      // ... to inventory
      packet->getU16();

      auto toInventoryId = packet->getU8();
      auto unknown = packet->getU16();
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("%s: Moving %u (countOrSubType %u) from %s (stackpos: %u) to inventoryId %u (unknown: %u)",
                __func__,
                itemId,
                countOrSubType,
                fromPosition.toString().c_str(),
                fromStackPos,
                toInventoryId,
                unknown);

      gameEngine_->moveItemFromPosToInv(playerId_, fromPosition, fromStackPos, itemId, countOrSubType, toInventoryId);
    }
    else
    {
      // ... to Tile
      auto toPosition = getPosition(packet);
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("%s: Moving %u (countOrSubType %u) from %s (stackpos: %u) to %s (unknown: %u)",
                __func__,
                itemId,
                countOrSubType,
                fromPosition.toString().c_str(),
                fromStackPos,
                toPosition.toString().c_str());

      gameEngine_->moveItemFromPosToPos(playerId_, fromPosition, fromStackPos, itemId, countOrSubType, toPosition);
    }
  }
}

void Protocol71::parseUseItem(IncomingPacket* packet)
{
  // There are two options here:
  if (packet->peekU16() == 0xFFFF)
  {
    // Use Item in inventory
    packet->getU16();
    auto inventoryIndex = packet->getU8();
    auto unknown = packet->getU16();
    auto itemId = packet->getU16();
    auto unknown2 = packet->getU16();

    LOG_DEBUG("%s: Item %u at inventory index: %u (unknown: %u, unknown2: %u)",
              __func__,
              itemId,
              inventoryIndex,
              unknown,
              unknown2);

    gameEngine_->useInvItem(playerId_, itemId, inventoryIndex);
  }
  else
  {
    // Use Item on Tile
    auto position = getPosition(packet);
    auto itemId = packet->getU16();
    auto stackPosition = packet->getU8();
    auto unknown = packet->getU8();

    LOG_DEBUG("%s: Item %u at Tile: %s stackPos: %u (unknown: %u)",
              __func__,
              itemId,
              position.toString().c_str(),
              stackPosition,
              unknown);

    gameEngine_->usePosItem(playerId_, itemId, position, stackPosition);
  }
}

void Protocol71::parseLookAt(IncomingPacket* packet)
{
  // There are two options here:
  if (packet->peekU16() == 0xFFFF)
  {
    // Look at Item in inventory
    packet->getU16();
    auto inventoryIndex = packet->getU8();
    auto unknown = packet->getU16();
    auto itemId = packet->getU16();
    auto unknown2 = packet->getU8();

    LOG_DEBUG("%s: Item %u at inventory index: %u (unknown: %u, unknown2: %u)",
              __func__,
              itemId,
              inventoryIndex,
              unknown,
              unknown2);

    gameEngine_->lookAtInvItem(playerId_, inventoryIndex, itemId);
  }
  else
  {
    // Look at Item on Tile
    auto position = getPosition(packet);
    auto itemId = packet->getU16();
    auto stackPos = packet->getU8();

    LOG_DEBUG("%s: Item %u at Tile: %s stackPos: %u",
              __func__,
              itemId,
              position.toString().c_str(),
              stackPos);

    gameEngine_->lookAtPosItem(playerId_, position, itemId, stackPos);
  }
}

void Protocol71::parseSay(IncomingPacket* packet)
{
  uint8_t type = packet->getU8();

  std::string receiver = "";
  uint16_t channelId = 0;

  switch (type)
  {
    case 0x06:  // PRIVATE
    case 0x0B:  // PRIVATE RED
      receiver = packet->getString();
      break;
    case 0x07:  // CHANNEL_Y
    case 0x0A:  // CHANNEL_R1
      channelId = packet->getU16();
      break;
    default:
      break;
  }

  std::string message = packet->getString();

  gameEngine_->say(playerId_, type, message, receiver, channelId);
}

void Protocol71::parseCancelMove(IncomingPacket* packet)
{
  gameEngine_->cancelMove(playerId_);
}

Position Protocol71::getPosition(IncomingPacket* packet) const
{
  auto x = packet->getU16();
  auto y = packet->getU16();
  auto z = packet->getU8();
  return Position(x, y, z);
}
