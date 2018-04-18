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

#include "protocol_71.h"

#include <cstdio>
#include <cstdint>

#include <algorithm>
#include <deque>
#include <utility>

#include "game_engine.h"
#include "server.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "world_interface.h"
#include "logger.h"
#include "tile.h"
#include "account.h"
#include "game_position.h"

Protocol71::Protocol71(const std::function<void(void)>& closeProtocol,
                       GameEngineQueue* gameEngineQueue,
                       ConnectionId connectionId,
                       Server* server,
                       AccountReader* accountReader)
  : closeProtocol_(closeProtocol),
    playerId_(Creature::INVALID_ID),
    gameEngineQueue_(gameEngineQueue),
    connectionId_(connectionId),
    server_(server),
    accountReader_(accountReader)
{
  knownCreatures_.fill(Creature::INVALID_ID);
}

void Protocol71::disconnected()
{
  // We may not send any more packets now
  server_ = nullptr;

  if (isLoggedIn())
  {
    // Despawn the player
    gameEngineQueue_->addTask(playerId_, [this](GameEngine* gameEngine)
    {
      gameEngine->despawn(playerId_);
    });
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
    const auto packetId = packet->getU8();
    switch (packetId)
    {
      case 0x14:
      {
        gameEngineQueue_->addTask(playerId_, [this](GameEngine* gameEngine)
        {
          gameEngine->despawn(playerId_);
        });
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
        gameEngineQueue_->addTask(playerId_, [this, packetId](GameEngine* gameEngine)
        {
          gameEngine->move(playerId_, static_cast<Direction>(packetId - 0x65));
        });
        break;
      }

      case 0x69:
      {
        gameEngineQueue_->addTask(playerId_, [this](GameEngine* gameEngine)
        {
          gameEngine->cancelMove(playerId_);
        });
        break;
      }

      case 0x6F:  // Player turn, North = 0
      case 0x70:  // East  = 1
      case 0x71:  // South = 2
      case 0x72:  // West  = 3
      {
        gameEngineQueue_->addTask(playerId_, [this, packetId](GameEngine* gameEngine)
        {
          gameEngine->turn(playerId_, static_cast<Direction>(packetId - 0x6F));
        });
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

      case 0x87:
      {
        parseCloseContainer(packet);
        break;
      }

      case 0x88:
      {
        parseOpenParentContainer(packet);
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
        // Note: this packet more likely means "stop all actions", not only moving
        //       so, maybe we should cancel all player's task here?
        gameEngineQueue_->addTask(playerId_, [this](GameEngine* gameEngine)
        {
          gameEngine->cancelMove(playerId_);
        });
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

void Protocol71::onCreatureSpawn(const WorldInterface& world_interface,
                                 const Creature& creature,
                                 const Position& position)
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
      addEquipment(player.getEquipment(), i, &packet);
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

void Protocol71::onCreatureDespawn(const WorldInterface& world_interface,
                                   const Creature& creature,
                                   const Position& position,
                                   int stackPos)
{
  (void)world_interface;

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
                                int oldStackPos,
                                const Position& newPosition)
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

void Protocol71::onCreatureTurn(const WorldInterface& world_interface,
                                const Creature& creature,
                                const Position& position,
                                int stackPos)
{
  (void)world_interface;

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
  packet.addU8(static_cast<std::uint8_t>(creature.getDirection()));
  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onCreatureSay(const WorldInterface& world_interface,
                               const Creature& creature,
                               const Position& position,
                               const std::string& message)
{
  (void)world_interface;

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

void Protocol71::onItemRemoved(const WorldInterface& world_interface, const Position& position, int stackPos)
{
  (void)world_interface;

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
  (void)world_interface;

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
  addEquipment(player.getEquipment(), inventoryIndex, &packet);
  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onOpenContainer(int clientContainerId, const Container& container, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  if (item.getItemType().maxitems == 0)
  {
    LOG_ERROR("%s: Container with ItemTypeId: %d has maxitems == 0", __func__, item.getItemTypeId());
    return;
  }

  LOG_DEBUG("%s: clientContainerId: %u", __func__, clientContainerId);

  OutgoingPacket packet;
  packet.addU8(0x6E);
  packet.addU8(clientContainerId);
  addItem(item, &packet);
  packet.addString(item.getItemType().name);
  packet.addU8(item.getItemType().maxitems);
  packet.addU8(container.parentContainerId == Container::INVALID_ID ? 0x00 : 0x01);
  packet.addU8(container.items.size());
  for (const auto* item : container.items)
  {
    packet.addU16(item->getItemTypeId());
    if (item->getItemType().isStackable)  // or splash or fluid container?
    {
      packet.addU8(item->getCount());
    }
  }
  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onCloseContainer(int clientContainerId)
{
  if (!isConnected())
  {
    return;
  }

  LOG_DEBUG("%s: clientContainerId: %u", __func__, clientContainerId);

  OutgoingPacket packet;
  packet.addU8(0x6F);
  packet.addU8(clientContainerId);
  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onContainerAddItem(int clientContainerId, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  LOG_DEBUG("%s: clientContainerId: %u, itemTypeId: %d", __func__, clientContainerId, item.getItemTypeId());

  OutgoingPacket packet;
  packet.addU8(0x70);
  packet.addU8(clientContainerId);
  addItem(item, &packet);
  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onContainerUpdateItem(int clientContainerId, int containerSlot, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  LOG_DEBUG("%s: clientContainerId: %u, containerSlot: %d, itemTypeId: %d",
            __func__,
            clientContainerId,
            containerSlot,
            item.getItemTypeId());

  OutgoingPacket packet;
  packet.addU8(0x71);
  packet.addU8(clientContainerId);
  packet.addU8(containerSlot);
  addItem(item, &packet);
  server_->sendPacket(connectionId_, std::move(packet));
}

void Protocol71::onContainerRemoveItem(int clientContainerId, int containerSlot)
{
  if (!isConnected())
  {
    return;
  }

  LOG_DEBUG("%s: clientContainerId: %u, containerSlot: %d",
            __func__,
            clientContainerId,
            containerSlot);

  OutgoingPacket packet;
  packet.addU8(0x72);
  packet.addU8(clientContainerId);
  packet.addU8(containerSlot);
  server_->sendPacket(connectionId_, std::move(packet));
}

// 0x13 default text, 0x11 login text
void Protocol71::sendTextMessage(int message_type, const std::string& message)
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

void Protocol71::addMapData(const WorldInterface& world_interface,
                            const Position& position,
                            int width,
                            int height,
                            OutgoingPacket* packet)
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
        addItem(*(*itemIt), packet);
        count++;
        ++itemIt;

        // if splash; add; count++

        // Add top Items
        while (count < 10 && itemIt != items.cend())
        {
          if (!(*itemIt)->getItemType().alwaysOnTop)
          {
            break;
          }

          addItem(*(*itemIt), packet);
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
          addItem(*(*itemIt), packet);
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
      // TODO(simon): Figure out how to handle this - related to "creatureId to remove" below?
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
  packet->addU8(static_cast<std::uint8_t>(creature.getDirection()));
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
  packet->addU16(item.getItemTypeId());
  if (item.getItemType().isStackable)
  {
    packet->addU8(item.getCount());
  }
  else if (item.getItemType().isMultitype)
  {
    // TODO(simon): getSubType???
    packet->addU8(0);
  }
}

void Protocol71::addEquipment(const Equipment& equipment, int inventoryIndex, OutgoingPacket* packet) const
{
  const auto* item = equipment.getItem(inventoryIndex);
  if (!item)
  {
    packet->addU8(0x79);  // No Item in this slot
    packet->addU8(inventoryIndex);
  }
  else
  {
    packet->addU8(0x78);
    packet->addU8(inventoryIndex);
    addItem(*item, packet);
  }
}

void Protocol71::parseLogin(IncomingPacket* packet)
{
  packet->getU8();  // Unknown (0x02)
  const auto client_os = packet->getU8();
  const auto client_version = packet->getU16();
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

  // Login OK, spawn player
  gameEngineQueue_->addTask(playerId_, [this, character_name](GameEngine* gameEngine)
  {
    gameEngine->spawn(character_name, this);
  });
}

void Protocol71::parseMoveClick(IncomingPacket* packet)
{
  std::deque<Direction> moves;
  const auto pathLength = packet->getU8();

  if (pathLength == 0)
  {
    LOG_ERROR("%s: Path length is zero!", __func__);
    return;
  }

  for (auto i = 0; i < pathLength; i++)
  {
    moves.push_back(static_cast<Direction>(packet->getU8()));
  }

  gameEngineQueue_->addTask(playerId_, [this, moves](GameEngine* gameEngine) mutable
  {
    gameEngine->movePath(playerId_, std::move(moves));
  });
}

void Protocol71::parseMoveItem(IncomingPacket* packet)
{
  const auto fromItemPosition = getItemPosition(packet);
  const auto toGamePosition = getGamePosition(packet);
  const auto count = packet->getU8();

  LOG_DEBUG("%s: from: %s, to: %s, count: %u",
            __func__,
            fromItemPosition.toString().c_str(),
            toGamePosition.toString().c_str(),
            count);

  gameEngineQueue_->addTask(playerId_, [this, fromItemPosition, toGamePosition, count](GameEngine* gameEngine)
  {
    gameEngine->moveItem(playerId_, fromItemPosition, toGamePosition, count);
  });
}

void Protocol71::parseUseItem(IncomingPacket* packet)
{
  const auto itemPosition = getItemPosition(packet);
  const auto newContainerId = packet->getU8();

  LOG_DEBUG("%s: itemPosition: %s, newContainerId: %u", __func__, itemPosition.toString().c_str(), newContainerId);

  gameEngineQueue_->addTask(playerId_, [this, itemPosition, newContainerId](GameEngine* gameEngine)
  {
    gameEngine->useItem(playerId_, itemPosition, newContainerId);
  });
}

void Protocol71::parseCloseContainer(IncomingPacket* packet)
{
  const auto clientContainerId = packet->getU8();

  LOG_DEBUG("%s: clientContainerId: %u", __func__, clientContainerId);

  gameEngineQueue_->addTask(playerId_, [this, clientContainerId](GameEngine* gameEngine)
  {
    gameEngine->closeContainer(playerId_, clientContainerId);
  });
}

void Protocol71::parseOpenParentContainer(IncomingPacket* packet)
{
  const auto clientContainerId = packet->getU8();

  LOG_DEBUG("%s: clientContainerId: %u", __func__, clientContainerId);

  gameEngineQueue_->addTask(playerId_, [this, clientContainerId](GameEngine* gameEngine)
  {
    gameEngine->openParentContainer(playerId_, clientContainerId);
  });
}

void Protocol71::parseLookAt(IncomingPacket* packet)
{
  const auto itemPosition = getItemPosition(packet);

  LOG_DEBUG("%s: itemPosition: %s", __func__, itemPosition.toString().c_str());

  gameEngineQueue_->addTask(playerId_, [this, itemPosition](GameEngine* gameEngine)
  {
    gameEngine->lookAt(playerId_, itemPosition);
  });
}

void Protocol71::parseSay(IncomingPacket* packet)
{
  const auto type = packet->getU8();

  std::string receiver = "";
  auto channelId = 0;

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

  const auto message = packet->getString();

  gameEngineQueue_->addTask(playerId_, [this, type, message, receiver, channelId](GameEngine* gameEngine)
  {
    gameEngine->say(playerId_, type, message, receiver, channelId);
  });
}

GamePosition Protocol71::getGamePosition(IncomingPacket* packet) const
{
  const auto x = packet->getU16();
  const auto y = packet->getU16();
  const auto z = packet->getU8();

  LOG_DEBUG("%s: x = 0x%04X, y = 0x%04X, z = 0x%02X", __func__, x, y, z);

  if (x != 0xFFFF)
  {
    // Positions have x not fully set
    return GamePosition(Position(x, y, z));
  }
  else if ((y & 0x40) == 0x00)
  {
    // Inventory have x fully set and 7th bit in y not set
    // Inventory slot is 4 lower bits in y
    return GamePosition(y & ~0x40);
  }
  else
  {
    // Container have x fully set and 7th bit in y set
    // Container id is lower 6 bits in y
    // Container slot is z
    const auto clientContainerId = y & ~0x40;
    return GamePosition(clientContainerId, z);
  }
}

ItemPosition Protocol71::getItemPosition(IncomingPacket* packet) const
{
  const auto gamePosition = getGamePosition(packet);
  const auto itemId = packet->getU16();
  const auto stackPosition = packet->getU8();

  return ItemPosition(gamePosition, itemId, stackPosition);
}
