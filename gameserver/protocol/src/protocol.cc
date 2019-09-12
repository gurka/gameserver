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

Protocol::Protocol(std::function<void(void)> close_protocol,
                   std::unique_ptr<Connection>&& connection,
                   const WorldInterface* world_interface,
                   GameEngineQueue* game_engine_queue,
                   AccountReader* account_reader)
  : m_close_protocol(std::move(close_protocol)),
    m_connection(std::move(connection)),
    m_world_interface(world_interface),
    m_game_engine_queue(game_engine_queue),
    m_account_reader(account_reader),
    m_player_id(Creature::INVALID_ID)
{
  m_known_creatures.fill(Creature::INVALID_ID);
  m_container_ids.fill(Item::INVALID_UNIQUE_ID);

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
  m_connection->init(callbacks);
}

void Protocol::onCreatureSpawn(const Creature& creature, const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;

  if (creature.getCreatureId() == m_player_id)
  {
    // We are spawning!
    const auto& player = static_cast<const Player&>(creature);
    const auto server_beat = 50;  // TODO(simon): customizable?

    // TODO(simon): Check if any of these can be reordered, e.g. move addWorldLight down
    protocol_helper::addLogin(m_player_id, server_beat, &packet);
    protocol_helper::addMapFull(*m_world_interface, position, &m_known_creatures, &packet);
    protocol_helper::addMagicEffect(position, 0x0A, &packet);
    protocol_helper::addPlayerStats(player, &packet);
    protocol_helper::addWorldLight(0x64, 0xD7, &packet);
    protocol_helper::addPlayerSkills(player, &packet);
    for (auto i = 1; i <= 10; i++)
    {
      protocol_helper::addEquipmentUpdated(player.getEquipment(), i, &packet);
    }
  }
  else
  {
    // Someone else spawned
    protocol_helper::addThingAdded(position, &creature, &m_known_creatures, &packet);
    protocol_helper::addMagicEffect(position, 0x0A, &packet);
  }

  m_connection->sendPacket(std::move(packet));
}

void Protocol::onCreatureDespawn(const Creature& creature, const Position& position, std::uint8_t stackpos)
{
  if (!isConnected())
  {
    if (creature.getCreatureId() == m_player_id)
    {
      // We are no longer in game and the connection has been closed, close the protocol
      m_player_id = Creature::INVALID_ID;
      m_close_protocol();  // WARNING: This instance is deleted after this call
    }
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addMagicEffect(position, 0x02, &packet);
  protocol_helper::addThingRemoved(position, stackpos, &packet);
  m_connection->sendPacket(std::move(packet));

  if (creature.getCreatureId() == m_player_id)
  {
    // This player despawned, close the connection gracefully
    // The protocol will be deleted as soon as the connection has been closed
    // (via onConnectionClosed callback)
    m_player_id = Creature::INVALID_ID;
    m_connection->close(false);
  }
}

void Protocol::onCreatureMove(const Creature& creature,
                              const Position& old_position,
                              std::uint8_t old_stackpos,
                              const Position& new_position)
{
  if (!isConnected())
  {
    return;
  }

  // Build outgoing packet
  OutgoingPacket packet;

  const auto& player_position = m_world_interface->getCreaturePosition(m_player_id);
  bool can_see_old_pos = canSee(player_position, old_position);
  bool can_see_new_pos = canSee(player_position, new_position);

  if (can_see_old_pos && can_see_new_pos)
  {
    protocol_helper::addThingMoved(old_position, old_stackpos, new_position, &packet);
  }
  else if (can_see_old_pos)
  {
    protocol_helper::addThingRemoved(old_position, old_stackpos, &packet);
  }
  else if (can_see_new_pos)
  {
    protocol_helper::addThingAdded(new_position, &creature, &m_known_creatures, &packet);
  }
  else
  {
    LOG_ERROR("%s: called, but this player cannot see neither old_position nor new_position: "
              "player_position: %s, old_position: %s, new_position: %s",
              __func__,
              player_position.toString().c_str(),
              old_position.toString().c_str(),
              new_position.toString().c_str());
    disconnect();
    return;
  }

  if (creature.getCreatureId() == m_player_id)
  {
    // Changing level is currently not supported
    if (old_position.getZ() != new_position.getZ())
    {
      LOG_ERROR("%s: changing level is not supported!", __func__);
      disconnect();
      return;
    }

    // This player moved, send new map data
    protocol_helper::addMap(*m_world_interface, old_position, new_position, &m_known_creatures, &packet);
  }

  m_connection->sendPacket(std::move(packet));
}

void Protocol::onCreatureTurn(const Creature& creature, const Position& position, std::uint8_t stackpos)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addThingChanged(position, stackpos, &creature, &m_known_creatures, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onCreatureSay(const Creature& creature, const Position& position, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addTalk(creature.getName(), 0x01, position, message, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onItemRemoved(const Position& position, std::uint8_t stackpos)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addThingRemoved(position, stackpos, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onItemAdded(const Item& item, const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addThingAdded(position, &item, nullptr, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onTileUpdate(const Position& position)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addTileUpdated(position, *m_world_interface, &m_known_creatures, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onEquipmentUpdated(const Player& player, std::uint8_t inventory_index)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addEquipmentUpdated(player.getEquipment(), inventory_index, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onOpenContainer(std::uint8_t new_container_id, const Container& container, const Item& item)
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

  // Set container_id
  setContainerId(new_container_id, item.getItemUniqueId());

  LOG_DEBUG("%s: new_container_id: %u", __func__, new_container_id);

  OutgoingPacket packet;
  protocol_helper::addContainerOpen(new_container_id, &item, container, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onCloseContainer(ItemUniqueId container_item_unique_id, bool reset_container_id)
{
  if (!isConnected())
  {
    return;
  }

  // Find container_id
  const auto container_id = getContainerId(container_item_unique_id);
  if (container_id == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with item_unique_id: %lu", __func__, container_item_unique_id);
    disconnect();
    return;
  }

  if (reset_container_id)
  {
    setContainerId(container_id, Item::INVALID_UNIQUE_ID);
  }

  LOG_DEBUG("%s: container_item_unique_id: %u -> container_id: %d", __func__, container_item_unique_id, container_id);

  OutgoingPacket packet;
  protocol_helper::addContainerClose(container_id, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onContainerAddItem(ItemUniqueId container_item_unique_id, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  const auto container_id = getContainerId(container_item_unique_id);
  if (container_id == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with item_unique_id: %lu", __func__, container_item_unique_id);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: container_item_unique_id: %u -> container_id: %d, itemTypeId: %d",
            __func__,
            container_item_unique_id,
            container_id,
            item.getItemTypeId());

  OutgoingPacket packet;
  protocol_helper::addContainerAddItem(container_id, &item, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onContainerUpdateItem(ItemUniqueId container_item_unique_id, std::uint8_t container_slot, const Item& item)
{
  if (!isConnected())
  {
    return;
  }

  const auto container_id = getContainerId(container_item_unique_id);
  if (container_id == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with item_unique_id: %lu", __func__, container_item_unique_id);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: container_item_unique_id: %u -> container_id: %d, container_slot: %d, itemTypeId: %d",
            __func__,
            container_item_unique_id,
            container_id,
            container_slot,
            item.getItemTypeId());

  OutgoingPacket packet;
  protocol_helper::addContainerUpdateItem(container_id, container_slot, &item, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::onContainerRemoveItem(ItemUniqueId container_item_unique_id, std::uint8_t container_slot)
{
  if (!isConnected())
  {
    return;
  }

  const auto container_id = getContainerId(container_item_unique_id);
  if (container_id == INVALID_CONTAINER_ID)
  {
    LOG_ERROR("%s: could not find an open container with item_unique_id: %lu", __func__, container_item_unique_id);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: container_item_unique_id: %u -> container_id: %d, container_slot: %d",
            __func__,
            container_item_unique_id,
            container_id,
            container_slot);

  OutgoingPacket packet;
  protocol_helper::addContainerRemoveItem(container_id, container_slot, &packet);
  m_connection->sendPacket(std::move(packet));
}

// 0x13 default text, 0x11 login text
void Protocol::sendTextMessage(std::uint8_t message_type, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addTextMessage(message_type, message, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::sendCancel(const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addTextMessage(0x14, message, &packet);
  m_connection->sendPacket(std::move(packet));
}

void Protocol::cancelMove()
{
  if (!isConnected())
  {
    return;
  }

  OutgoingPacket packet;
  protocol_helper::addCancelMove(&packet);
  m_connection->sendPacket(std::move(packet));
}

bool Protocol::hasContainerOpen(ItemUniqueId item_unique_id) const
{
  return getContainerId(item_unique_id) != INVALID_CONTAINER_ID;
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
  m_connection->close(true);
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
    auto packet_type = packet->getU8();
    if (packet_type == 0x0A)
    {
      parseLogin(packet);
    }
    else
    {
      LOG_ERROR("%s: Expected login packet but received packet type: 0x%X", __func__, packet_type);
      disconnect();
    }

    return;
  }

  while (!packet->isEmpty())
  {
    const auto packet_id = packet->getU8();
    switch (packet_id)
    {
      case 0x14:
      {
        m_game_engine_queue->addTask(m_player_id, [this](GameEngine* game_engine)
        {
          game_engine->despawn(m_player_id);
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
        m_game_engine_queue->addTask(m_player_id, [this, packet_id](GameEngine* game_engine)
        {
          game_engine->move(m_player_id, static_cast<Direction>(packet_id - 0x65));
        });
        break;
      }

      case 0x69:
      {
        m_game_engine_queue->addTask(m_player_id, [this](GameEngine* game_engine)
        {
          game_engine->cancelMove(m_player_id);
        });
        break;
      }

      case 0x6F:  // Player turn, North = 0
      case 0x70:  // East  = 1
      case 0x71:  // South = 2
      case 0x72:  // West  = 3
      {
        m_game_engine_queue->addTask(m_player_id, [this, packet_id](GameEngine* game_engine)
        {
          game_engine->turn(m_player_id, static_cast<Direction>(packet_id - 0x6F));
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
        m_game_engine_queue->addTask(m_player_id, [this](GameEngine* game_engine)
        {
          game_engine->cancelMove(m_player_id);
        });
        break;
      }

      default:
      {
        LOG_ERROR("Unknown packet from player id: %d, packet id: 0x%X", m_player_id, packet_id);
        return;  // Don't read any more, even though there might be more packets that we can parse
      }
    }
  }
}

void Protocol::onDisconnected()
{
  // We are no longer connected, so erase the connection
  m_connection.reset();

  // If we are not logged in to the gameworld then we can erase the protocol
  if (!isLoggedIn())
  {
    m_close_protocol();  // Note that this instance is deleted during this call
  }
  else
  {
    // We need to tell the gameengine to despawn us
    m_game_engine_queue->addTask(m_player_id, [this](GameEngine* game_engine)
    {
      game_engine->despawn(m_player_id);
    });
  }
}

void Protocol::parseLogin(IncomingPacket* packet)
{
  const auto login = protocol_helper::getLogin(packet);

  LOG_DEBUG("Client OS: %d Client version: %d Character: %s Password: %s",
            login.client_os,
            login.client_version,
            login.character_name.c_str(),
            login.password.c_str());

  // Check if character exists
  if (!m_account_reader->characterExists(login.character_name))
  {
    OutgoingPacket packet;
    protocol_helper::addLoginFailed("Invalid character.", &packet);
    m_connection->sendPacket(std::move(packet));
    m_connection->close(false);
    return;
  }

  // Check if password is correct
  if (!m_account_reader->verifyPassword(login.character_name, login.password))
  {
    OutgoingPacket packet;
    protocol_helper::addLoginFailed("Invalid password.", &packet);
    m_connection->sendPacket(std::move(packet));
    m_connection->close(false);
    return;
  }

  // Login OK, spawn player
  m_game_engine_queue->addTask(m_player_id, [this, character_name = login.character_name](GameEngine* game_engine)
  {
    if (!game_engine->spawn(character_name, this))
    {
      OutgoingPacket packet;
      protocol_helper::addLoginFailed("Could not spawn player.", &packet);
      m_connection->sendPacket(std::move(packet));
      m_connection->close(false);
    }
  });
}

void Protocol::parseMoveClick(IncomingPacket* packet)
{
  auto move = protocol_helper::getMoveClick(packet);
  if (move.path.empty())
  {
    LOG_ERROR("%s: Path length is zero!", __func__);
    disconnect();
    return;
  }

  m_game_engine_queue->addTask(m_player_id, [this, path = std::move(move.path)](GameEngine* game_engine) mutable
  {
    game_engine->movePath(m_player_id, std::move(path));
  });
}

void Protocol::parseMoveItem(IncomingPacket* packet)
{
  const auto move = protocol_helper::getMoveItem(&m_container_ids, packet);

  LOG_DEBUG("%s: from: %s, to: %s, count: %u",
            __func__,
            move.from_item_position.toString().c_str(),
            move.to_game_position.toString().c_str(),
            move.count);

  m_game_engine_queue->addTask(m_player_id, [this, move](GameEngine* game_engine)
  {
    game_engine->moveItem(m_player_id, move.from_item_position, move.to_game_position, move.count);
  });
}

void Protocol::parseUseItem(IncomingPacket* packet)
{
  const auto use_item = protocol_helper::getUseItem(&m_container_ids, packet);

  LOG_DEBUG("%s: item_position: %s, new_container_id: %u",
            __func__,
            use_item.item_position.toString().c_str(),
            use_item.new_container_id);

  m_game_engine_queue->addTask(m_player_id, [this, use_item](GameEngine* game_engine)
  {
    game_engine->useItem(m_player_id, use_item.item_position, use_item.new_container_id);
  });
}

void Protocol::parseCloseContainer(IncomingPacket* packet)
{
  const auto close = protocol_helper::getCloseContainer(packet);
  const auto item_unique_id = getContainerItemUniqueId(close.container_id);
  if (item_unique_id == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: container_id: %d does not map to a valid ItemUniqueId", __func__, close.container_id);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: container_id: %d -> item_unique_id: %u", __func__, close.container_id, item_unique_id);

  m_game_engine_queue->addTask(m_player_id, [this, item_unique_id](GameEngine* game_engine)
  {
    game_engine->closeContainer(m_player_id, item_unique_id);
  });
}

void Protocol::parseOpenParentContainer(IncomingPacket* packet)
{
  const auto open_parent = protocol_helper::getOpenParentContainer(packet);
  const auto item_unique_id = getContainerItemUniqueId(open_parent.container_id);
  if (item_unique_id == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: container_id: %d does not map to a valid ItemUniqueId", __func__, open_parent.container_id);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: container_id: %d -> item_unique_id: %u", __func__, open_parent.container_id, item_unique_id);

  m_game_engine_queue->addTask(m_player_id, [this, item_unique_id, open_parent](GameEngine* game_engine)
  {
    game_engine->openParentContainer(m_player_id, item_unique_id, open_parent.container_id);
  });
}

void Protocol::parseLookAt(IncomingPacket* packet)
{
  const auto look_at = protocol_helper::getLookAt(&m_container_ids, packet);

  LOG_DEBUG("%s: item_position: %s", __func__, look_at.item_position.toString().c_str());

  m_game_engine_queue->addTask(m_player_id, [this, look_at](GameEngine* game_engine)
  {
    game_engine->lookAt(m_player_id, look_at.item_position);
  });
}

void Protocol::parseSay(IncomingPacket* packet)
{
  const auto say = protocol_helper::getSay(packet);

  m_game_engine_queue->addTask(m_player_id, [this, say](GameEngine* game_engine)
  {
    // TODO(simon): probably different calls depending on say.type
    game_engine->say(m_player_id, say.type, say.message, say.receiver, say.channel_id);
  });
}

void Protocol::setContainerId(std::uint8_t container_id, ItemUniqueId item_unique_id)
{
  m_container_ids[container_id] = item_unique_id;
}

std::uint8_t Protocol::getContainerId(ItemUniqueId item_unique_id) const
{
  const auto it = std::find(m_container_ids.cbegin(),
                            m_container_ids.cend(),
                            item_unique_id);
  if (it != m_container_ids.cend())
  {
    return std::distance(m_container_ids.cbegin(), it);
  }
  return INVALID_CONTAINER_ID;
}

ItemUniqueId Protocol::getContainerItemUniqueId(std::uint8_t container_id) const
{
  if (container_id >= 64)
  {
    LOG_ERROR("%s: invalid container_id: %d", __func__, container_id);
    return Item::INVALID_UNIQUE_ID;
  }

  return m_container_ids.at(container_id);
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
