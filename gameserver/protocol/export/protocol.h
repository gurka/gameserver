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

class WorldInterface;

class Protocol : public gameengine::PlayerCtrl
{
 public:
  Protocol(std::function<void(void)> close_protocol,
           std::unique_ptr<network::Connection>&& connection,
           const WorldInterface* world_interface,
           gameengine::GameEngineQueue* game_engine_queue,
           account::AccountReader* account_reader);

  // Delete copy constructors
  Protocol(const Protocol&) = delete;
  Protocol& operator=(const Protocol&) = delete;

  // Called by World (from CreatureCtrl)
  void onCreatureSpawn(const Creature& creature,
                       const Position& position) override;
  void onCreatureDespawn(const Creature& creature,
                         const Position& position,
                         std::uint8_t stackpos) override;
  void onCreatureMove(const Creature& creature,
                      const Position& old_position,
                      std::uint8_t old_stackpos,
                      const Position& new_position) override;
  void onCreatureTurn(const Creature& creature,
                      const Position& position,
                      std::uint8_t stackpos) override;
  void onCreatureSay(const Creature& creature,
                     const Position& position,
                     const std::string& message) override;

  void onItemRemoved(const Position& position,
                     std::uint8_t stackpos) override;
  void onItemAdded(const Item& item,
                   const Position& position) override;

  void onTileUpdate(const Position& position) override;

  // Called by GameEngine (from PlayerCtrl)
  CreatureId getPlayerId() const override { return m_player_id; }
  void setPlayerId(CreatureId player_id) override { m_player_id = player_id; }
  void onEquipmentUpdated(const gameengine::Player& player, std::uint8_t inventory_index) override;
  void onOpenContainer(std::uint8_t new_container_id, const gameengine::Container& container, const Item& item) override;
  void onCloseContainer(ItemUniqueId container_item_unique_id, bool reset_container_id) override;
  void onContainerAddItem(ItemUniqueId container_item_unique_id, const Item& item) override;
  void onContainerUpdateItem(ItemUniqueId container_item_unique_id, std::uint8_t container_slot, const Item& item) override;
  void onContainerRemoveItem(ItemUniqueId container_item_unique_id, std::uint8_t container_slot) override;
  void sendTextMessage(std::uint8_t message_type, const std::string& message) override;
  void sendCancel(const std::string& message) override;
  void cancelMove() override;

  // Called by ContainerManager (from PlayerCtrl)
  const std::array<ItemUniqueId, 64>& getContainerIds() const override { return m_container_ids; }
  bool hasContainerOpen(ItemUniqueId item_unique_id) const override;

 private:
  bool isLoggedIn() const { return m_player_id != Creature::INVALID_ID; }
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
  void setContainerId(std::uint8_t container_id, ItemUniqueId item_unique_id);
  std::uint8_t getContainerId(ItemUniqueId item_unique_id) const;
  ItemUniqueId getContainerItemUniqueId(std::uint8_t container_id) const;

  // Other helpers
  bool canSee(const Position& player_position, const Position& to_position);

  std::function<void(void)> m_close_protocol;
  std::unique_ptr<network::Connection> m_connection;
  const WorldInterface* m_world_interface;
  gameengine::GameEngineQueue* m_game_engine_queue;
  account::AccountReader* m_account_reader;

  CreatureId m_player_id;

  std::array<CreatureId, 64> m_known_creatures;

  // Known/opened containers
  // clientContainerId maps to a container's ItemUniqueId
  static constexpr std::uint8_t INVALID_CONTAINER_ID = -1;
  std::array<ItemUniqueId, 64> m_container_ids;
};

#endif  // PROTOCOL_EXPORT_PROTOCOL_H_
