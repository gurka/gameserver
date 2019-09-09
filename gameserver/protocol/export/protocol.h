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

#ifndef WORLDSERVER_SRC_PROTOCOL_H_
#define WORLDSERVER_SRC_PROTOCOL_H_

#include "player_ctrl.h"

#include <array>
#include <functional>
#include <string>
#include <memory>

// gameengine
#include "player.h"
#include "game_position.h"
#include "container.h"

// world
#include "creature.h"
#include "position.h"
#include "item.h"

class Connection;
class IncomingPacket;
class OutgoingPacket;
class GameEngineQueue;
class AccountReader;
class WorldInterface;

class Protocol : public PlayerCtrl
{
 public:
  Protocol(const std::function<void(void)>& closeProtocol,
             std::unique_ptr<Connection>&& connection,
             const WorldInterface* worldInterface,
             GameEngineQueue* gameEngineQueue,
             AccountReader* accountReader);

  // Delete copy constructors
  Protocol(const Protocol&) = delete;
  Protocol& operator=(const Protocol&) = delete;

  // Called by World (from CreatureCtrl)
  void onCreatureSpawn(const Creature& creature,
                       const Position& position) override;
  void onCreatureDespawn(const Creature& creature,
                         const Position& position,
                         std::uint8_t stackPos) override;
  void onCreatureMove(const Creature& creature,
                      const Position& oldPosition,
                      std::uint8_t oldStackPos,
                      const Position& newPosition) override;
  void onCreatureTurn(const Creature& creature,
                      const Position& position,
                      std::uint8_t stackPos) override;
  void onCreatureSay(const Creature& creature,
                     const Position& position,
                     const std::string& message) override;

  void onItemRemoved(const Position& position,
                     std::uint8_t stackPos) override;
  void onItemAdded(const Item& item,
                   const Position& position) override;

  void onTileUpdate(const Position& position) override;

  // Called by GameEngine (from PlayerCtrl)
  CreatureId getPlayerId() const override { return playerId_; }
  void setPlayerId(CreatureId playerId) override { playerId_ = playerId; }
  void onEquipmentUpdated(const Player& player, std::uint8_t inventoryIndex) override;
  void onOpenContainer(std::uint8_t newContainerId, const Container& container, const Item& item) override;
  void onCloseContainer(ItemUniqueId containerItemUniqueId, bool resetContainerId) override;
  void onContainerAddItem(ItemUniqueId containerItemUniqueId, const Item& item) override;
  void onContainerUpdateItem(ItemUniqueId containerItemUniqueId, std::uint8_t containerSlot, const Item& item) override;
  void onContainerRemoveItem(ItemUniqueId containerItemUniqueId, std::uint8_t containerSlot) override;
  void sendTextMessage(std::uint8_t message_type, const std::string& message) override;
  void sendCancel(const std::string& message) override;
  void cancelMove() override;

  // Called by ContainerManager (from PlayerCtrl)
  const std::array<ItemUniqueId, 64>& getContainerIds() const override { return containerIds_; }
  bool hasContainerOpen(ItemUniqueId itemUniqueId) const override;

 private:
  bool isLoggedIn() const { return playerId_ != Creature::INVALID_ID; }
  bool isConnected() const { return static_cast<bool>(connection_); }
  void disconnect() const;

  // Connection callbacks
  void parsePacket(IncomingPacket* packet);
  void onDisconnected();

  // Functions to parse IncomingPackets
  void parseLogin(IncomingPacket* packet);
  void parseMoveClick(IncomingPacket* packet);
  void parseMoveItem(IncomingPacket* packet);
  void parseUseItem(IncomingPacket* packet);
  void parseCloseContainer(IncomingPacket* packet);
  void parseOpenParentContainer(IncomingPacket* packet);
  void parseLookAt(IncomingPacket* packet);
  void parseSay(IncomingPacket* packet);

  // Helper functions for containerId
  void setContainerId(std::uint8_t containerId, ItemUniqueId itemUniqueId);
  std::uint8_t getContainerId(ItemUniqueId itemUniqueId) const;
  ItemUniqueId getContainerItemUniqueId(std::uint8_t containerId) const;

  std::function<void(void)> closeProtocol_;
  std::unique_ptr<Connection> connection_;
  const WorldInterface* worldInterface_;
  GameEngineQueue* gameEngineQueue_;
  AccountReader* accountReader_;

  CreatureId playerId_;

  std::array<CreatureId, 64> knownCreatures_;

  // Known/opened containers
  // clientContainerId maps to a container's ItemUniqueId
  static constexpr std::uint8_t INVALID_CONTAINER_ID = -1;
  std::array<ItemUniqueId, 64> containerIds_;
};

#endif  // WORLDSERVER_SRC_PROTOCOL_H_
