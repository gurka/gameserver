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

constexpr std::uint8_t Protocol::INVALID_CONTAINER_ID;

Protocol::Protocol(const std::function<void(void)>& closeProtocol,
                   std::unique_ptr<Connection>&& connection,
                   const WorldInterface* worldInterface,
                   GameEngineQueue* gameEngineQueue,
                   AccountReader* accountReader)
  : closeProtocol_(closeProtocol),
    connection_(std::move(connection)),
    worldInterface_(worldInterface),
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

void Protocol::onCreatureSpawn(const Creature& creature, const Position& position)
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
    const auto serverBeat = 50;  // TODO(simon): customizable?

    // TODO(simon): Check if any of these can be reordered, e.g. move addWorldLight down
    ProtocolHelper::addLogin(playerId_, serverBeat, &packet);
    ProtocolHelper::addMapFull(*worldInterface_, position, &knownCreatures_, &packet);
    ProtocolHelper::addMagicEffect(position, 0x0A, &packet);
    ProtocolHelper::addPlayerStats(player, &packet);
    ProtocolHelper::addWorldLight(0x64, 0xD7, &packet);
    ProtocolHelper::addPlayerSkills(player, &packet);
    for (auto i = 1; i <= 10; i++)
    {
      ProtocolHelper::addEquipmentUpdated(player.getEquipment(), i, &packet);
    }
  }
  else
  {
    // Someone else spawned
    ProtocolHelper::addThingAdded(position, &creature, &knownCreatures_, &packet);
    ProtocolHelper::addMagicEffect(position, 0x0A, &packet);
  }

  connection_->sendPacket(std::move(packet));
}

void Protocol::onCreatureDespawn(const Creature& creature, const Position& position, std::uint8_t stackPos)
{
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
  ProtocolHelper::addThingRemoved(position, stackPos, &packet);
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

void Protocol::onCreatureMove(const Creature& creature,
                              const Position& oldPosition,
                              std::uint8_t oldStackPos,
                              const Position& newPosition)
{
  if (!isConnected())
  {
    return;
  }

  // Build outgoing packet
  OutgoingPacket packet;

  const auto& player_position = worldInterface_->getCreaturePosition(playerId_);
  bool canSeeOldPos = canSee(player_position, oldPosition);
  bool canSeeNewPos = canSee(player_position, newPosition);

  if (canSeeOldPos && canSeeNewPos)
  {
    ProtocolHelper::addThingMoved(oldPosition, oldStackPos, newPosition, &packet);
  }
  else if (canSeeOldPos)
  {
    ProtocolHelper::addThingRemoved(oldPosition, oldStackPos, &packet);
  }
  else if (canSeeNewPos)
  {
    ProtocolHelper::addThingAdded(newPosition, &creature, &knownCreatures_, &packet);
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
    ProtocolHelper::addMap(*worldInterface_, oldPosition, newPosition, &knownCreatures_, &packet);
  }

  connection_->sendPacket(std::move(packet));
}

void Protocol::onCreatureTurn(const Creature& creature, const Position& position, std::uint8_t stackPos)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addThingChanged(position, stackPos, &creature, &knownCreatures_, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onCreatureSay(const Creature& creature, const Position& position, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addTalk(creature.getName(), 0x01, position, message, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onItemRemoved(const Position& position, std::uint8_t stackPos)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addThingRemoved(position, stackPos, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onItemAdded(const Item& item, const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addThingAdded(position, &item, nullptr, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onTileUpdate(const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addTileUpdated(position, *worldInterface_, &knownCreatures_, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onEquipmentUpdated(const Player& player, std::uint8_t inventoryIndex)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addEquipmentUpdated(player.getEquipment(), inventoryIndex, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onOpenContainer(std::uint8_t newContainerId, const Container& container, const Item& item)
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
  ProtocolHelper::addContainerOpen(newContainerId, &item, container, &packet);
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
  ProtocolHelper::addContainerClose(containerId, &packet);
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
  ProtocolHelper::addContainerAddItem(containerId, &item, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onContainerUpdateItem(ItemUniqueId containerItemUniqueId, std::uint8_t containerSlot, const Item& item)
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
  ProtocolHelper::addContainerUpdateItem(containerId, containerSlot, &item, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::onContainerRemoveItem(ItemUniqueId containerItemUniqueId, std::uint8_t containerSlot)
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
  ProtocolHelper::addContainerRemoveItem(containerId, containerSlot, &packet);
  connection_->sendPacket(std::move(packet));
}

// 0x13 default text, 0x11 login text
void Protocol::sendTextMessage(std::uint8_t message_type, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addTextMessage(message_type, message, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::sendCancel(const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addTextMessage(0x14, message, &packet);
  connection_->sendPacket(std::move(packet));
}

void Protocol::cancelMove()
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  ProtocolHelper::addCancelMove(&packet);
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
    OutgoingPacket packet;
    ProtocolHelper::addLoginFailed("Invalid character.", &packet);
    connection_->sendPacket(std::move(packet));
    connection_->close(false);
    return;
  }

  // Check if password is correct
  if (!accountReader_->verifyPassword(character_name, password))
  {
    OutgoingPacket packet;
    ProtocolHelper::addLoginFailed("Invalid password.", &packet);
    connection_->sendPacket(std::move(packet));
    connection_->close(false);
    return;
  }

  // Login OK, spawn player
  gameEngineQueue_->addTask(playerId_, [this, character_name](GameEngine* gameEngine)
  {
    if (!gameEngine->spawn(character_name, this))
    {
      OutgoingPacket packet;
      ProtocolHelper::addLoginFailed("Could not spawn player.", &packet);
      connection_->sendPacket(std::move(packet));
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
  std::uint16_t channelId = 0;

  switch (type)
  {
    case 0x06:  // PRIVATE
    case 0x0B:  // PRIVATE RED
      packet->get(&receiver);
      break;
    case 0x07:  // CHANNEL_Y
    case 0x0A:  // CHANNEL_R1
      packet->get(&channelId);
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

void Protocol::setContainerId(std::uint8_t containerId, ItemUniqueId itemUniqueId)
{
  containerIds_[containerId] = itemUniqueId;
}

std::uint8_t Protocol::getContainerId(ItemUniqueId itemUniqueId) const
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

ItemUniqueId Protocol::getContainerItemUniqueId(std::uint8_t containerId) const
{
  if (containerId >= 64)
  {
    LOG_ERROR("%s: invalid containerId: %d", __func__, containerId);
    return Item::INVALID_UNIQUE_ID;
  }

  return containerIds_.at(containerId);
}

bool Protocol::canSee(const Position& player_position, const Position& to_position)
{
  // Note: client displays 15x11 tiles, but it know about 18x14 tiles.
  //
  //       Client know about one extra row north, one extra column west
  //       two extra rows south and two extra rows east.
  //
  //       This function returns true if to_position is visible from player_position
  //       with regards to what the client (player_position) knows about, e.g. 18x14 tiles.
  //
  //
  //     00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18
  //     ________________________________________________________
  // 00 |   _______________________________________________      |
  // 01 |  |                                               |     |
  // 02 |  |                                               |     |
  // 03 |  |                                               |     |
  // 04 |  |                                               |     |
  // 05 |  |                                               |     |
  // 06 |  |                                               |     |
  // 07 |  |                                               |     |
  // 08 |  |                                               |     |
  // 09 |  |                                               |     |
  // 10 |  |                                               |     |
  // 11 |  |                                               |     |
  // 12 |  |_______________________________________________|     |
  // 13 |                                                        |
  // 14 |________________________________________________________|

  return to_position.getX() >= player_position.getX() - 8 &&
         to_position.getX() <= player_position.getX() + 9 &&
         to_position.getY() >= player_position.getY() - 6 &&
         to_position.getY() <= player_position.getY() + 7;
}
