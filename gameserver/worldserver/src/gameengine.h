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

#ifndef WORLDSERVER_GAMEENGINE_H_
#define WORLDSERVER_GAMEENGINE_H_

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/asio.hpp>  //NOLINT

#include "taskqueue.h"
#include "player.h"
#include "position.h"

class OutgoingPacket;
class Protocol;
class World;

class GameEngine
{
 public:
  GameEngine(boost::asio::io_service* io_service,
             const std::string& loginMessage,
             World* world);

  // Not copyable
  GameEngine(const GameEngine&) = delete;
  GameEngine& operator=(const GameEngine&) = delete;

  template<class F, class... Args>
  void addTask(F&& f, Args&&... args)
  {
    taskQueue_.addTask(std::bind(f, this, args...));
  }

  // These functions should only be called via addTask
  void playerSpawn(const std::string& name, Protocol* protocol);
  void playerDespawn(CreatureId creatureId);

  void playerMove(CreatureId creatureId, Direction direction);
  void playerMovePath(CreatureId creatureId, const std::deque<Direction>& path);
  void playerCancelMove(CreatureId creatureId);
  void playerTurn(CreatureId creatureId, Direction direction);

  void playerSay(CreatureId creatureId,
                 uint8_t type,
                 const std::string& message,
                 const std::string& receiver,
                 uint16_t channelId);

  void playerMoveItemFromPosToPos(CreatureId creatureId,
                                  const Position& fromPosition,
                                  int fromStackPos,
                                  int itemId,
                                  int count,
                                  const Position& toPosition);
  void playerMoveItemFromPosToInv(CreatureId creatureId,
                                  const Position& fromPosition,
                                  int fromStackPos,
                                  int itemId,
                                  int count,
                                  int inventoryId);
  void playerMoveItemFromInvToPos(CreatureId creatureId,
                                  int fromInventoryId,
                                  int itemId,
                                  int count,
                                  const Position& toPosition);
  void playerMoveItemFromInvToInv(CreatureId creatureId,
                                  int fromInventoryId,
                                  int itemId,
                                  int count,
                                  int toInventoryId);

  void playerUseInvItem(CreatureId creatureId, int itemId, int inventoryIndex);
  void playerUsePosItem(CreatureId creatureId, int itemId, const Position& position, int stackPos);

  void playerLookAtInvItem(CreatureId creatureId, int inventoryIndex, ItemId itemId);
  void playerLookAtPosItem(CreatureId creatureId, const Position& position, ItemId itemId, int stackPos);

 private:
  void playerMovePathStep(CreatureId creatureId);

  // Use these instead of the unordered_maps directly
  Player& getPlayer(CreatureId creatureId) { return playerProtocol_.at(creatureId).player; }
  Protocol* getProtocol(CreatureId creatureId) { return playerProtocol_.at(creatureId).protocol; }

  // Task stuff
  using TaskFunction = std::function<void(void)>;
  TaskQueue<TaskFunction> taskQueue_;

  struct PlayerProtocol
  {
    Player player;
    Protocol* protocol;
  };
  std::unordered_map<CreatureId, PlayerProtocol> playerProtocol_;

  std::string loginMessage_;

  World* world_;
};

#endif  // WORLDSERVER_GAMEENGINE_H_
