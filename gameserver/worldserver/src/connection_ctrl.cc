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

#include "connection_ctrl.h"

#include <cstdio>

#include <algorithm>
#include <deque>
#include <utility>

// network
#include "connection.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"

// gameengine
#include "game_engine.h"
#include "game_engine_queue.h"
#include "game_position.h"

// world
#include "world.h"
#include "tile.h"
#include "item.h"

// utils
#include "logger.h"

// account
#include "account.h"

// protocol
#include "protocol_common.h"
#include "protocol_server.h"

using namespace protocol::server;  // NOLINT yes we want it all

ConnectionCtrl::ConnectionCtrl(std::function<void(void)> close_protocol,
                               std::unique_ptr<network::Connection>&& connection,
                               const world::World* world,
                               gameengine::GameEngineQueue* game_engine_queue,
                               account::AccountReader* account_reader)
    : m_close_protocol(std::move(close_protocol)),
      m_connection(std::move(connection)),
      m_world(world),
      m_game_engine_queue(game_engine_queue),
      m_account_reader(account_reader),
      m_player_id(common::Creature::INVALID_ID)
{
  m_known_creatures.fill(common::Creature::INVALID_ID);
  m_container_ids.fill(common::Item::INVALID_UNIQUE_ID);

  network::Connection::Callbacks callbacks
  {
    // onPacketReceived
    [this](network::IncomingPacket* packet)
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
  m_connection->init(callbacks, false);
}

void ConnectionCtrl::onCreatureSpawn(const common::Creature& creature, const common::Position& position)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;

  if (creature.getCreatureId() == m_player_id)
  {
    // We are spawning!
    const auto& player = static_cast<const gameengine::Player&>(creature);
    const auto server_beat = 50;  // TODO(simon): customizable?

    // TODO(simon): Check if any of these can be reordered, e.g. move addWorldLight down
    addLogin(m_player_id, server_beat, &packet);
    addMapFull(*m_world, position, &m_known_creatures, &packet);
    addMagicEffect(position, 0x0A, &packet);
    addPlayerStats(player, &packet);
    addWorldLight(0x64, 0xD7, &packet);
    addPlayerSkills(player, &packet);
    for (auto i = 1; i <= 10; i++)
    {
      addEquipmentUpdated(player.getEquipment(), i, &packet);
    }
  }
  else
  {
    // Someone else spawned
    addThingAdded(position, &creature, &m_known_creatures, &packet);
    addMagicEffect(position, 0x0A, &packet);
  }

  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onCreatureDespawn(const common::Creature& creature, const common::Position& position, std::uint8_t stackpos)
{
  if (!isConnected())
  {
    if (creature.getCreatureId() == m_player_id)
    {
      // We are no longer in game and the connection has been closed, close the protocol
      m_player_id = common::Creature::INVALID_ID;
      m_close_protocol();  // WARNING: This instance is deleted after this call
    }
    return;
  }

  network::OutgoingPacket packet;
  addMagicEffect(position, 0x02, &packet);
  addThingRemoved(position, stackpos, &packet);
  m_connection->sendPacket(std::move(packet));

  if (creature.getCreatureId() == m_player_id)
  {
    // This player despawned, close the connection gracefully
    // The protocol will be deleted as soon as the connection has been closed
    // (via onConnectionClosed callback)
    m_player_id = common::Creature::INVALID_ID;
    m_connection->close(false);
  }
}

void ConnectionCtrl::onCreatureMove(const common::Creature& creature,
                              const common::Position& old_position,
                              std::uint8_t old_stackpos,
                              const common::Position& new_position)
{
  if (!isConnected())
  {
    return;
  }

  // Build outgoing packet
  network::OutgoingPacket packet;

  const auto* player_position = m_world->getCreaturePosition(m_player_id);
  if (!player_position)
  {
    LOG_ERROR("%s: invalid player_position", __func__);
    return;
  }

  bool can_see_old_pos = canSee(*player_position, old_position);
  bool can_see_new_pos = canSee(*player_position, new_position);

  if (can_see_old_pos && can_see_new_pos)
  {
    addThingMoved(old_position, old_stackpos, new_position, &packet);
  }
  else if (can_see_old_pos)
  {
    addThingRemoved(old_position, old_stackpos, &packet);
  }
  else if (can_see_new_pos)
  {
    addThingAdded(new_position, &creature, &m_known_creatures, &packet);
  }
  else
  {
    LOG_ERROR("%s: called, but this player cannot see neither old_position nor new_position: "
              "player_position: %s, old_position: %s, new_position: %s",
              __func__,
              player_position->toString().c_str(),
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
    addMap(*m_world, old_position, new_position, &m_known_creatures, &packet);
  }

  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onCreatureTurn(const common::Creature& creature, const common::Position& position, std::uint8_t stackpos)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addThingChanged(position, stackpos, &creature, &m_known_creatures, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onCreatureSay(const common::Creature& creature, const common::Position& position, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addTalk(creature.getName(), 0x01, position, message, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onItemRemoved(const common::Position& position, std::uint8_t stackpos)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addThingRemoved(position, stackpos, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onItemAdded(const common::Item& item, const common::Position& position)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addThingAdded(position, &item, nullptr, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onTileUpdate(const common::Position& position)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addTileUpdated(position, *m_world, &m_known_creatures, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onEquipmentUpdated(const gameengine::Player& player, std::uint8_t inventory_index)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addEquipmentUpdated(player.getEquipment(), inventory_index, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onOpenContainer(std::uint8_t new_container_id, const gameengine::Container& container, const common::Item& item)
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

  network::OutgoingPacket packet;
  addContainerOpen(new_container_id, &item, container, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onCloseContainer(common::ItemUniqueId container_item_unique_id, bool reset_container_id)
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
    setContainerId(container_id, common::Item::INVALID_UNIQUE_ID);
  }

  LOG_DEBUG("%s: container_item_unique_id: %u -> container_id: %d", __func__, container_item_unique_id, container_id);

  network::OutgoingPacket packet;
  addContainerClose(container_id, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onContainerAddItem(common::ItemUniqueId container_item_unique_id, const common::Item& item)
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

  network::OutgoingPacket packet;
  addContainerAddItem(container_id, &item, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onContainerUpdateItem(common::ItemUniqueId container_item_unique_id, std::uint8_t container_slot, const common::Item& item)
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

  network::OutgoingPacket packet;
  addContainerUpdateItem(container_id, container_slot, &item, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::onContainerRemoveItem(common::ItemUniqueId container_item_unique_id, std::uint8_t container_slot)
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

  network::OutgoingPacket packet;
  addContainerRemoveItem(container_id, container_slot, &packet);
  m_connection->sendPacket(std::move(packet));
}

// 0x13 default text, 0x11 login text
void ConnectionCtrl::sendTextMessage(std::uint8_t message_type, const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addTextMessage(message_type, message, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::sendCancel(const std::string& message)
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addTextMessage(0x14, message, &packet);
  m_connection->sendPacket(std::move(packet));
}

void ConnectionCtrl::cancelMove()
{
  if (!isConnected())
  {
    return;
  }

  network::OutgoingPacket packet;
  addCancelMove(&packet);
  m_connection->sendPacket(std::move(packet));
}

bool ConnectionCtrl::hasContainerOpen(common::ItemUniqueId item_unique_id) const
{
  return getContainerId(item_unique_id) != INVALID_CONTAINER_ID;
}

void ConnectionCtrl::disconnect() const
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

void ConnectionCtrl::parsePacket(network::IncomingPacket* packet)
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
        m_game_engine_queue->addTask(m_player_id, [this](gameengine::GameEngine* game_engine)
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
        m_game_engine_queue->addTask(m_player_id, [this, packet_id](gameengine::GameEngine* game_engine)
        {
          game_engine->move(m_player_id, static_cast<common::Direction>(packet_id - 0x65));
        });
        break;
      }

      case 0x69:
      {
        m_game_engine_queue->addTask(m_player_id, [this](gameengine::GameEngine* game_engine)
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
        m_game_engine_queue->addTask(m_player_id, [this, packet_id](gameengine::GameEngine* game_engine)
        {
          game_engine->turn(m_player_id, static_cast<common::Direction>(packet_id - 0x6F));
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
        m_game_engine_queue->addTask(m_player_id, [this](gameengine::GameEngine* game_engine)
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

void ConnectionCtrl::onDisconnected()
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
    m_game_engine_queue->addTask(m_player_id, [this](gameengine::GameEngine* game_engine)
    {
      game_engine->despawn(m_player_id);
    });
  }
}

void ConnectionCtrl::parseLogin(network::IncomingPacket* packet)
{
  const auto login = getLogin(packet);

  LOG_DEBUG("Client OS: %d Client version: %d Character: %s Password: %s",
            login.client_os,
            login.client_version,
            login.character_name.c_str(),
            login.password.c_str());

  // Check if character exists
  if (!m_account_reader->characterExists(login.character_name))
  {
    network::OutgoingPacket packet;
    addLoginFailed("Invalid character.", &packet);
    m_connection->sendPacket(std::move(packet));
    m_connection->close(false);
    return;
  }

  // Check if password is correct
  if (!m_account_reader->verifyPassword(login.character_name, login.password))
  {
    network::OutgoingPacket packet;
    addLoginFailed("Invalid password.", &packet);
    m_connection->sendPacket(std::move(packet));
    m_connection->close(false);
    return;
  }

  // Login OK, spawn player
  m_game_engine_queue->addTask(m_player_id, [this, character_name = login.character_name](gameengine::GameEngine* game_engine)
  {
    if (!game_engine->spawn(character_name, this))
    {
      network::OutgoingPacket packet;
      addLoginFailed("Could not spawn player.", &packet);
      m_connection->sendPacket(std::move(packet));
      m_connection->close(false);
    }
  });
}

void ConnectionCtrl::parseMoveClick(network::IncomingPacket* packet)
{
  auto move = getMoveClick(packet);
  if (move.path.empty())
  {
    LOG_ERROR("%s: Path length is zero!", __func__);
    disconnect();
    return;
  }

  m_game_engine_queue->addTask(m_player_id, [this, path = std::move(move.path)](gameengine::GameEngine* game_engine) mutable
  {
    game_engine->movePath(m_player_id, std::move(path));
  });
}

void ConnectionCtrl::parseMoveItem(network::IncomingPacket* packet)
{
  const auto move = getMoveItem(&m_container_ids, packet);

  LOG_DEBUG("%s: from: %s, to: %s, count: %u",
            __func__,
            move.from_item_position.toString().c_str(),
            move.to_game_position.toString().c_str(),
            move.count);

  m_game_engine_queue->addTask(m_player_id, [this, move](gameengine::GameEngine* game_engine)
  {
    game_engine->moveItem(m_player_id, move.from_item_position, move.to_game_position, move.count);
  });
}

void ConnectionCtrl::parseUseItem(network::IncomingPacket* packet)
{
  const auto use_item = getUseItem(&m_container_ids, packet);

  LOG_DEBUG("%s: item_position: %s, new_container_id: %u",
            __func__,
            use_item.item_position.toString().c_str(),
            use_item.new_container_id);

  m_game_engine_queue->addTask(m_player_id, [this, use_item](gameengine::GameEngine* game_engine)
  {
    game_engine->useItem(m_player_id, use_item.item_position, use_item.new_container_id);
  });
}

void ConnectionCtrl::parseCloseContainer(network::IncomingPacket* packet)
{
  const auto close = getCloseContainer(packet);
  const auto item_unique_id = getContainerItemUniqueId(close.container_id);
  if (item_unique_id == common::Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: container_id: %d does not map to a valid ItemUniqueId", __func__, close.container_id);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: container_id: %d -> item_unique_id: %u", __func__, close.container_id, item_unique_id);

  m_game_engine_queue->addTask(m_player_id, [this, item_unique_id](gameengine::GameEngine* game_engine)
  {
    game_engine->closeContainer(m_player_id, item_unique_id);
  });
}

void ConnectionCtrl::parseOpenParentContainer(network::IncomingPacket* packet)
{
  const auto open_parent = getOpenParentContainer(packet);
  const auto item_unique_id = getContainerItemUniqueId(open_parent.container_id);
  if (item_unique_id == common::Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: container_id: %d does not map to a valid ItemUniqueId", __func__, open_parent.container_id);
    disconnect();
    return;
  }

  LOG_DEBUG("%s: container_id: %d -> item_unique_id: %u", __func__, open_parent.container_id, item_unique_id);

  m_game_engine_queue->addTask(m_player_id, [this, item_unique_id, open_parent](gameengine::GameEngine* game_engine)
  {
    game_engine->openParentContainer(m_player_id, item_unique_id, open_parent.container_id);
  });
}

void ConnectionCtrl::parseLookAt(network::IncomingPacket* packet)
{
  const auto look_at = getLookAt(&m_container_ids, packet);

  LOG_DEBUG("%s: item_position: %s", __func__, look_at.item_position.toString().c_str());

  m_game_engine_queue->addTask(m_player_id, [this, look_at](gameengine::GameEngine* game_engine)
  {
    game_engine->lookAt(m_player_id, look_at.item_position);
  });
}

void ConnectionCtrl::parseSay(network::IncomingPacket* packet)
{
  const auto say = getSay(packet);

  m_game_engine_queue->addTask(m_player_id, [this, say](gameengine::GameEngine* game_engine)
  {
    // TODO(simon): probably different calls depending on say.type
    game_engine->say(m_player_id, say.type, say.message, say.receiver, say.channel_id);
  });
}

void ConnectionCtrl::setContainerId(std::uint8_t container_id, common::ItemUniqueId item_unique_id)
{
  m_container_ids[container_id] = item_unique_id;
}

std::uint8_t ConnectionCtrl::getContainerId(common::ItemUniqueId item_unique_id) const
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

common::ItemUniqueId ConnectionCtrl::getContainerItemUniqueId(std::uint8_t container_id) const
{
  if (container_id >= 64)
  {
    LOG_ERROR("%s: invalid container_id: %d", __func__, container_id);
    return common::Item::INVALID_UNIQUE_ID;
  }

  return m_container_ids.at(container_id);
}

bool ConnectionCtrl::canSee(const common::Position& player_position, const common::Position& to_position)
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
