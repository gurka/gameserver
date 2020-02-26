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

#ifndef WORLDSERVER_SRC_CONNECTION_CTRL_H_
#define WORLDSERVER_SRC_CONNECTION_CTRL_H_

#include <array>
#include <functional>
#include <string>
#include <memory>

// gameengine
#include "player_ctrl.h"
#include "player.h"
#include "game_position.h"
#include "container.h"

// world
#include "creature.h"
#include "position.h"
#include "item.h"

namespace account
{
class AccountReader;
}

namespace gameengine
{
class GameEngineQueue;
}

namespace network
{
class Connection;
class IncomingPacket;
}

namespace world
{
class World;
}

class ConnectionCtrl : public gameengine::PlayerCtrl
{
 public:
  ConnectionCtrl(std::function<void(void)> close_protocol,
                 std::unique_ptr<network::Connection>&& connection,
                 const world::World* world,
                 gameengine::GameEngineQueue* game_engine_queue,
                 account::AccountReader* account_reader);

  // Delete copy constructors
  ConnectionCtrl(const ConnectionCtrl&) = delete;
  ConnectionCtrl& operator=(const ConnectionCtrl&) = delete;

  // Called by World (from CreatureCtrl)
  void onCreatureSpawn(const common::Creature& creature,
                       const common::Position& position) override;
  void onCreatureDespawn(const common::Creature& creature,
                         const common::Position& position,
                         std::uint8_t stackpos) override;
  void onCreatureMove(const common::Creature& creature,
                      const common::Position& old_position,
                      std::uint8_t old_stackpos,
                      const common::Position& new_position) override;
  void onCreatureTurn(const common::Creature& creature,
                      const common::Position& position,
                      std::uint8_t stackpos) override;
  void onCreatureSay(const common::Creature& creature,
                     const common::Position& position,
                     const std::string& message) override;

  void onItemRemoved(const common::Position& position,
                     std::uint8_t stackpos) override;
  void onItemAdded(const common::Item& item,
                   const common::Position& position) override;

  void onTileUpdate(const common::Position& position) override;

  // Called by GameEngine (from PlayerCtrl)
  common::CreatureId getPlayerId() const override { return m_player_id; }
  void setPlayerId(common::CreatureId player_id) override { m_player_id = player_id; }
  void onEquipmentUpdated(const gameengine::Player& player, std::uint8_t inventory_index) override;
  void onOpenContainer(std::uint8_t new_container_id, const gameengine::Container& container, const common::Item& item) override;
  void onCloseContainer(common::ItemUniqueId container_item_unique_id, bool reset_container_id) override;
  void onContainerAddItem(common::ItemUniqueId container_item_unique_id, const common::Item& item) override;
  void onContainerUpdateItem(common::ItemUniqueId container_item_unique_id, std::uint8_t container_slot, const common::Item& item) override;
  void onContainerRemoveItem(common::ItemUniqueId container_item_unique_id, std::uint8_t container_slot) override;
  void sendTextMessage(std::uint8_t message_type, const std::string& message) override;
  void sendCancel(const std::string& message) override;
  void cancelMove() override;

  // Called by ContainerManager (from PlayerCtrl)
  const std::array<common::ItemUniqueId, 64>& getContainerIds() const override { return m_container_ids; }
  bool hasContainerOpen(common::ItemUniqueId item_unique_id) const override;

 private:
  bool isLoggedIn() const { return m_player_id != common::Creature::INVALID_ID; }
  bool isConnected() const { return static_cast<bool>(m_connection); }
  void disconnect() const;

  // Connection callbacks
  void parsePacket(network::IncomingPacket* packet);
  void onDisconnected();

  // Functions to parse IncomingPackets
  void parseLogin(network::IncomingPacket* packet);
  void parseMoveClick(network::IncomingPacket* packet);
  void parseMoveItem(network::IncomingPacket* packet);
  void parseUseItem(network::IncomingPacket* packet);
  void parseCloseContainer(network::IncomingPacket* packet);
  void parseOpenParentContainer(network::IncomingPacket* packet);
  void parseLookAt(network::IncomingPacket* packet);
  void parseSay(network::IncomingPacket* packet);

  // Helper functions for containerId
  void setContainerId(std::uint8_t container_id, common::ItemUniqueId item_unique_id);
  std::uint8_t getContainerId(common::ItemUniqueId item_unique_id) const;
  common::ItemUniqueId getContainerItemUniqueId(std::uint8_t container_id) const;

  // Other helpers
  static bool canSee(const common::Position& player_position, const common::Position& to_position);

  std::function<void(void)> m_close_protocol;
  std::unique_ptr<network::Connection> m_connection;
  const world::World* m_world;
  gameengine::GameEngineQueue* m_game_engine_queue;
  account::AccountReader* m_account_reader;

  common::CreatureId m_player_id;

  std::array<common::CreatureId, 64> m_known_creatures;

  // Known/opened containers
  // clientContainerId maps to a container's ItemUniqueId
  static constexpr std::uint8_t INVALID_CONTAINER_ID = -1;
  std::array<common::ItemUniqueId, 64> m_container_ids;
};

#endif  // WORLDSERVER_SRC_CONNECTION_CTRL_H_
