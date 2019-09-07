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

#include "protocol.h"

#include <cstdio>

#include <algorithm>
#include <deque>
#include <utility>

#include "protocol_helper.h"

// network
#include "connection.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"

// gameengine
#include "game_engine.h"
#include "game_engine_queue.h"
#include "game_position.h"

// world
#include "world_interface.h"
#include "tile.h"
#include "item.h"

// utils
#include "logger.h"

// account
#include "account.h"

constexpr int Protocol::INVALID_CONTAINER_ID;

Protocol::Protocol(const std::function<void(void)>& closeProtocol,
                   std::unique_ptr<Connection>&& connection,
                   GameEngineQueue* gameEngineQueue,
                   AccountReader* accountReader)
  : closeProtocol_(closeProtocol),
    connection_(std::move(connection)),
    gameEngineQueue_(gameEngineQueue),
    accountReader_(accountReader),
    playerId_(Creature::INVALID_ID)
{
  knownCreatures_.fill(Creature::INVALID_ID);
  containerIds_.fill(Item::INVALID_UNIQUE_ID);

  Connection::Callbacks callbacks
  {
    // onPacketReceived
    [this](IncomingPacket* packet)
    {
      LOG_DEBUG("onPacketReceived");
      parsePacket(packet);
    },

    // onDisconnected
    [this]()
    {
      LOG_DEBUG("onDisconnected");
      onDisconnected();
    }
  };
  connection_->init(callbacks);
}

void Protocol::onCreatureSpawn(const WorldInterface& world_interface,
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
    packet.addU16(50);  // Server beat, 50hz
                        // TODO(simon): customizable?

    // TODO(simon): Check if any of these can be reordered, e.g. move addWorldLight down
    ProtocolHelper::addFullMapData(world_interface, position, &knownCreatures_, &packet);
    ProtocolHelper::addMagicEffect(position, 0x0A, &packet);
    ProtocolHelper::addPlayerStats(player, &packet);
    ProtocolHelper::addWorldLight(0x64, 0xD7, &packet);
    ProtocolHelper::addPlayerSkills(player, &packet);
    for (auto i = 1; i <= 10; i++)
    {
      ProtocolHelper::addEquipment(player.getEquipment(), i, &packet);
    }
  }
  else
  {
    // Someone else spawned
    packet.addU8(0x6A);
    ProtocolHelper::addPosition(position, &packet);
    ProtocolHelper::addCreature(creature, &knownCreatures_, &packet);
    ProtocolHelper::addMagicEffect(position, 0x0A, &packet);
  }

  connection_->sendPacket(std::move(packet));
}

void Protocol::onCreatureDespawn(const WorldInterface& world_interface,
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
      playerId_ = Creature::INVALID_ID;
      closeProtocol_();  // WARNING: This instance is deleted after this call
    }
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addMagicEffect(position, 0x02, &packet);
  packet.addU8(0x6C);
  ProtocolHelper::addPosition(position, &packet);
  packet.addU8(stackPos);
  connection_->sendPacket(std::move(packet));

  if (creature.getCreatureId() == playerId_)
  {
    // This player despawned, close the connection gracefully
    // The protocol will be deleted as soon as the connection has been closed
    // (via onConnectionClosed callback)
    playerId_ = Creature::INVALID_ID;
    connection_->close(false);
  }
}

void Protocol::onCreatureMove(const WorldInterface& world_interface,
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
  bool canSeeOldPos = ProtocolHelper::canSee(player_position, oldPosition);
  bool canSeeNewPos = ProtocolHelper::canSee(player_position, newPosition);

  if (canSeeOldPos && canSeeNewPos)
  {
    packet.addU8(0x6D);
    ProtocolHelper::addPosition(oldPosition, &packet);
    packet.addU8(oldStackPos);
    ProtocolHelper::addPosition(newPosition, &packet);
  }
  else if (canSeeOldPos)
  {
    packet.addU8(0x6C);
    ProtocolHelper::addPosition(oldPosition, &packet);
    packet.addU8(oldStackPos);
  }
  else if (canSeeNewPos)
  {
    packet.addU8(0x6A);
    ProtocolHelper::addPosition(newPosition, &packet);
    ProtocolHelper::addCreature(creature, &knownCreatures_, &packet);
  }
  else
  {
    LOG_ERROR("%s: called, but this player cannot see neither oldPosition nor newPosition: "
              "player_position: %s, oldPosition: %s, newPosition: %s",
              __func__,
              player_position.toString().c_str(),
              oldPosition.toString().c_str(),
              newPosition.toString().c_str());
    disconnect();
    return;
  }

  if (creature.getCreatureId() == playerId_)
  {
    // Changing level is currently not supported
    if (oldPosition.getZ() != newPosition.getZ())
    {
      LOG_ERROR("%s: changing level is not supported!", __func__);
      disconnect();
      return;
    }

    // This player moved, send new map data
    if (oldPosition.getY() > newPosition.getY())
    {
      // Get north block
      packet.addU8(0x65);
      ProtocolHelper::addMapData(world_interface,
                                 Position(oldPosition.getX() - 8, newPosition.getY() - 6, oldPosition.getZ()),
                                 18,
                                 1,
                                 &knownCreatures_,
                                 &packet);
    }
    else if (oldPosition.getY() < newPosition.getY())
    {
      // Get south block
      packet.addU8(0x67);
      ProtocolHelper::addMapData(world_interface,
                                 Position(oldPosition.getX() - 8, newPosition.getY() + 7, oldPosition.getZ()),
                                 18,
                                 1,
                                 &knownCreatures_,
                                 &packet);
    }

    if (oldPosition.getX() > newPosition.getX())
    {
      // Get west block
      packet.addU8(0x68);
      ProtocolHelper::addMapData(world_interface,
                                 Position(newPosition.getX() - 8, newPosition.getY() - 6, oldPosition.getZ()),
                                 1,
                                 14,
                                 &knownCreatures_,
                                 &packet);
    }
    else if (oldPosition.getX() < newPosition.getX())
    {
      // Get west block
      packet.addU8(0x66);
      ProtocolHelper::addMapData(world_interface,
                                 Position(newPosition.getX() + 9, newPosition.getY() - 6, oldPosition.getZ()),
                                 1,
                                 14,
                                 &knownCreatures_,
                                 &packet);
    }
  }

  connection_->sendPacket(std::move(packet));
}

void Protocol::onCreatureTurn(const WorldInterface& world_interface,
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
  ProtocolHelper::addPosition(position, &packet);
  packet.addU8(stackPos);
  packet.addU8(0x63);
  packet.addU8(0x00);
  packet.addU32(creature.getCreatureId());
  packet.addU8(static_cast<std::uint8_t>(creature.getDirection()));
  connection_->sendPacket(std::move(packet));
}

void Protocol::onCreatureSay(const WorldInterface& world_interface,
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
  ProtocolHelper::addPosition(position, &packet);
  packet.addString(message);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onItemRemoved(const WorldInterface& world_interface, const Position& position, int stackPos)
{
  (void)world_interface;

  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  packet.addU8(0x6C);
  ProtocolHelper::addPosition(position, &packet);
  packet.addU8(stackPos);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onItemAdded(const WorldInterface& world_interface, const Item& item, const Position& position)
{
  (void)world_interface;

  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  packet.addU8(0x6A);
  ProtocolHelper::addPosition(position, &packet);
  ProtocolHelper::addItem(item, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onTileUpdate(const WorldInterface& world_interface, const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  packet.addU8(0x69);
  ProtocolHelper::addPosition(position, &packet);
  ProtocolHelper::addMapData(world_interface, position, 1, 1, &knownCreatures_, &packet);
  packet.addU8(0x00);
  packet.addU8(0xFF);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onEquipmentUpdated(const Player& player, int inventoryIndex)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addEquipment(player.getEquipment(), inventoryIndex, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onOpenContainer(int newContainerId, const Container& container, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  if (item.getItemType().maxitems == 0)
  {
    LOG_ERROR("%s: Container with ItemTypeId: %d has maxitems == 0", __func__, item.getItemTypeId());
    disconnect();
    return;
  }

  // Set containerId
  setContainerId(newContainerId, item.getItemUniqueId());

  LOG_DEBUG("%s: newContainerId: %u", __func__, newContainerId);

  OutgoingPacket packet;
  packet.addU8(0x6E);
  packet.addU8(newContainerId);
  ProtocolHelper::addItem(item, &packet);
  packet.addString(item.getItemType().name);
  packet.addU8(item.getItemType().maxitems);
  packet.addU8(container.parentItemUniqueId == Item::INVALID_UNIQUE_ID ? 0x00 : 0x01);
  packet.addU8(container.items.size());
  for (const auto* item : container.items)
  {
    packet.addU16(item->getItemTypeId());
    if (item->getItemType().isStackable)  // or splash or fluid container?
    {
      packet.addU8(item->getCount());
    }
  }
  connection_->sendPacket(std::move(packet));
}

void Protocol::onCloseContainer(ItemUniqueId containerItemUniqueId, bool resetContainerId)
{
  if (!isConnected())
  {
    return;
  }

  // Find containerId
  const auto containerId = getContainerId(containerItemUniqueId);
  if (containerId == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with itemUniqueId: %lu", __func__, containerItemUniqueId);
    disconnect();
    return;
  }

  if (resetContainerId)
  {
    setContainerId(containerId, Item::INVALID_UNIQUE_ID);
  }

  LOG_DEBUG("%s: containerItemUniqueId: %u -> containerId: %d", __func__, containerItemUniqueId, containerId);

  OutgoingPacket packet;
  packet.addU8(0x6F);
  packet.addU8(containerId);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onContainerAddItem(ItemUniqueId containerItemUniqueId, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  const auto containerId = getContainerId(containerItemUniqueId);
  if (containerId == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with itemUniqueId: %lu", __func__, containerItemUniqueId);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: containerItemUniqueId: %u -> containerId: %d, itemTypeId: %d",
            __func__,
            containerItemUniqueId,
            containerId,
            item.getItemTypeId());

  OutgoingPacket packet;
  packet.addU8(0x70);
  packet.addU8(containerId);
  ProtocolHelper::addItem(item, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onContainerUpdateItem(ItemUniqueId containerItemUniqueId, int containerSlot, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  const auto containerId = getContainerId(containerItemUniqueId);
  if (containerId == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with itemUniqueId: %lu", __func__, containerItemUniqueId);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: containerItemUniqueId: %u -> containerId: %d, containerSlot: %d, itemTypeId: %d",
            __func__,
            containerItemUniqueId,
            containerId,
            containerSlot,
            item.getItemTypeId());

  OutgoingPacket packet;
  packet.addU8(0x71);
  packet.addU8(containerId);
  packet.addU8(containerSlot);
  ProtocolHelper::addItem(item, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onContainerRemoveItem(ItemUniqueId containerItemUniqueId, int containerSlot)
{
  if (!isConnected())
  {
    return;
  }

  const auto containerId = getContainerId(containerItemUniqueId);
  if (containerId == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with itemUniqueId: %lu", __func__, containerItemUniqueId);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: containerItemUniqueId: %u -> containerId: %d, containerSlot: %d",
            __func__,
            containerItemUniqueId,
            containerId,
            containerSlot);

  OutgoingPacket packet;
  packet.addU8(0x72);
  packet.addU8(containerId);
  packet.addU8(containerSlot);
  connection_->sendPacket(std::move(packet));
}

// 0x13 default text, 0x11 login text
void Protocol::sendTextMessage(int message_type, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  packet.addU8(0xB4);
  packet.addU8(message_type);
  packet.addString(message);
  connection_->sendPacket(std::move(packet));
}

void Protocol::sendCancel(const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  packet.addU8(0xB4);
  packet.addU8(0x14);
  packet.addString(message);
  connection_->sendPacket(std::move(packet));
}

void Protocol::cancelMove()
{
  OutgoingPacket packet;
  packet.addU8(0xB5);
  connection_->sendPacket(std::move(packet));
}

bool Protocol::hasContainerOpen(ItemUniqueId itemUniqueId) const
{
  return getContainerId(itemUniqueId) != INVALID_CONTAINER_ID;
}

void Protocol::disconnect() const
{
  // Called when the user sent something bad
  if (!isConnected())
  {
    LOG_ERROR("%s: called when not connected", __func__);
    return;
  }

  // onDisconnect callback will handle the rest
  connection_->close(true);
}

void Protocol::parsePacket(IncomingPacket* packet)
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
      disconnect();
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

void Protocol::onDisconnected()
{
  // We are no longer connected, so erase the connection
  connection_.reset();

  // If we are not logged in to the gameworld then we can erase the protocol
  if (!isLoggedIn())
  {
    closeProtocol_();  // Note that this instance is deleted during this call
  }
  else
  {
    // We need to tell the gameengine to despawn us
    gameEngineQueue_->addTask(playerId_, [this](GameEngine* gameEngine)
    {
      gameEngine->despawn(playerId_);
    });
  }
}

void Protocol::parseLogin(IncomingPacket* packet)
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
    connection_->sendPacket(std::move(response));
    connection_->close(false);
    return;
  }

  // Check if password is correct
  if (!accountReader_->verifyPassword(character_name, password))
  {
    OutgoingPacket response;
    response.addU8(0x14);
    response.addString("Invalid password.");
    connection_->sendPacket(std::move(response));
    connection_->close(false);
    return;
  }

  // Login OK, spawn player
  gameEngineQueue_->addTask(playerId_, [this, character_name](GameEngine* gameEngine)
  {
    if (!gameEngine->spawn(character_name, this))
    {
      OutgoingPacket response;
      response.addU8(0x14);
      response.addString("Could not spawn player.");
      connection_->sendPacket(std::move(response));
      connection_->close(false);
    }
  });
}

void Protocol::parseMoveClick(IncomingPacket* packet)
{
  std::deque<Direction> moves;
  const auto pathLength = packet->getU8();

  if (pathLength == 0)
  {
    LOG_ERROR("%s: Path length is zero!", __func__);
    disconnect();
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

void Protocol::parseMoveItem(IncomingPacket* packet)
{
  const auto fromItemPosition = ProtocolHelper::getItemPosition(&containerIds_, packet);
  const auto toGamePosition = ProtocolHelper::getGamePosition(&containerIds_, packet);
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

void Protocol::parseUseItem(IncomingPacket* packet)
{
  const auto itemPosition = ProtocolHelper::getItemPosition(&containerIds_, packet);
  const auto newContainerId = packet->getU8();

  LOG_DEBUG("%s: itemPosition: %s, newContainerId: %u", __func__, itemPosition.toString().c_str(), newContainerId);

  gameEngineQueue_->addTask(playerId_, [this, itemPosition, newContainerId](GameEngine* gameEngine)
  {
    gameEngine->useItem(playerId_, itemPosition, newContainerId);
  });
}

void Protocol::parseCloseContainer(IncomingPacket* packet)
{
  const auto containerId = packet->getU8();
  const auto itemUniqueId = getContainerItemUniqueId(containerId);
  if (itemUniqueId == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: containerId: %d does not map to a valid ItemUniqueId", __func__, containerId);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: containerId: %d -> itemUniqueId: %u", __func__, containerId, itemUniqueId);

  gameEngineQueue_->addTask(playerId_, [this, itemUniqueId](GameEngine* gameEngine)
  {
    gameEngine->closeContainer(playerId_, itemUniqueId);
  });
}

void Protocol::parseOpenParentContainer(IncomingPacket* packet)
{
  const auto containerId = packet->getU8();
  const auto itemUniqueId = getContainerItemUniqueId(containerId);
  if (itemUniqueId == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: containerId: %d does not map to a valid ItemUniqueId", __func__, containerId);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: containerId: %d -> itemUniqueId: %u", __func__, containerId, itemUniqueId);

  gameEngineQueue_->addTask(playerId_, [this, itemUniqueId, containerId](GameEngine* gameEngine)
  {
    gameEngine->openParentContainer(playerId_, itemUniqueId, containerId);
  });
}

void Protocol::parseLookAt(IncomingPacket* packet)
{
  const auto itemPosition = ProtocolHelper::getItemPosition(&containerIds_, packet);

  LOG_DEBUG("%s: itemPosition: %s", __func__, itemPosition.toString().c_str());

  gameEngineQueue_->addTask(playerId_, [this, itemPosition](GameEngine* gameEngine)
  {
    gameEngine->lookAt(playerId_, itemPosition);
  });
}

void Protocol::parseSay(IncomingPacket* packet)
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

void Protocol::setContainerId(int containerId, ItemUniqueId itemUniqueId)
{
  containerIds_[containerId] = itemUniqueId;
}

int Protocol::getContainerId(ItemUniqueId itemUniqueId) const
{
  const auto it = std::find(containerIds_.cbegin(),
                            containerIds_.cend(),
                            itemUniqueId);
  if (it != containerIds_.cend())
  {
    return std::distance(containerIds_.cbegin(), it);
  }
  else
  {
    return INVALID_CONTAINER_ID;
  }
}

ItemUniqueId Protocol::getContainerItemUniqueId(int containerId) const
{
  if (containerId < 0 || containerId >= 64)
  {
    LOG_ERROR("%s: invalid containerId: %d", __func__, containerId);
    return Item::INVALID_UNIQUE_ID;
  }

  return containerIds_.at(containerId);
}
