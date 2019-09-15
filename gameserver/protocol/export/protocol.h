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

#ifndef PROTOCOL_EXPORT_PROTOCOL_H_
#define PROTOCOL_EXPORT_PROTOCOL_H_

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

class Protocol : public gameengine::PlayerCtrl
{
 public:
  Protocol(std::function<void(void)> close_protocol,
           std::unique_ptr<network::Connection>&& connection,
           const world::World* world,
           gameengine::GameEngineQueue* game_engine_queue,
           account::AccountReader* account_reader);

  // Delete copy constructors
  Protocol(const Protocol&) = delete;
  Protocol& operator=(const Protocol&) = delete;

  // Called by World (from CreatureCtrl)
  void onCreatureSpawn(const world::Creature& creature,
                       const world::Position& position) override;
  void onCreatureDespawn(const world::Creature& creature,
                         const world::Position& position,
                         std::uint8_t stackpos) override;
  void onCreatureMove(const world::Creature& creature,
                      const world::Position& old_position,
                      std::uint8_t old_stackpos,
                      const world::Position& new_position) override;
  void onCreatureTurn(const world::Creature& creature,
                      const world::Position& position,
                      std::uint8_t stackpos) override;
  void onCreatureSay(const world::Creature& creature,
                     const world::Position& position,
                     const std::string& message) override;

  void onItemRemoved(const world::Position& position,
                     std::uint8_t stackpos) override;
  void onItemAdded(const world::Item& item,
                   const world::Position& position) override;

  void onTileUpdate(const world::Position& position) override;

  // Called by GameEngine (from PlayerCtrl)
  world::CreatureId getPlayerId() const override { return m_player_id; }
  void setPlayerId(world::CreatureId player_id) override { m_player_id = player_id; }
  void onEquipmentUpdated(const gameengine::Player& player, std::uint8_t inventory_index) override;
  void onOpenContainer(std::uint8_t new_container_id, const gameengine::Container& container, const world::Item& item) override;
  void onCloseContainer(world::ItemUniqueId container_item_unique_id, bool reset_container_id) override;
  void onContainerAddItem(world::ItemUniqueId container_item_unique_id, const world::Item& item) override;
  void onContainerUpdateItem(world::ItemUniqueId container_item_unique_id, std::uint8_t container_slot, const world::Item& item) override;
  void onContainerRemoveItem(world::ItemUniqueId container_item_unique_id, std::uint8_t container_slot) override;
  void sendTextMessage(std::uint8_t message_type, const std::string& message) override;
  void sendCancel(const std::string& message) override;
  void cancelMove() override;

  // Called by ContainerManager (from PlayerCtrl)
  const std::array<world::ItemUniqueId, 64>& getContainerIds() const override { return m_container_ids; }
  bool hasContainerOpen(world::ItemUniqueId item_unique_id) const override;

 private:
  bool isLoggedIn() const { return m_player_id != world::Creature::INVALID_ID; }
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
  void setContainerId(std::uint8_t container_id, world::ItemUniqueId item_unique_id);
  std::uint8_t getContainerId(world::ItemUniqueId item_unique_id) const;
  world::ItemUniqueId getContainerItemUniqueId(std::uint8_t container_id) const;

  // Other helpers
  bool canSee(const world::Position& player_position, const world::Position& to_position);

  std::function<void(void)> m_close_protocol;
  std::unique_ptr<network::Connection> m_connection;
  const world::World* m_world;
  gameengine::GameEngineQueue* m_game_engine_queue;
  account::AccountReader* m_account_reader;

  world::CreatureId m_player_id;

  std::array<world::CreatureId, 64> m_known_creatures;

  // Known/opened containers
  // clientContainerId maps to a container's ItemUniqueId
  static constexpr std::uint8_t INVALID_CONTAINER_ID = -1;
  std::array<world::ItemUniqueId, 64> m_container_ids;
};

#endif  // PROTOCOL_EXPORT_PROTOCOL_H_
