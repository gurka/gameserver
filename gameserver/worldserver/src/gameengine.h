/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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

#include <boost/asio.hpp>  //NOLINT

#include "world.h"
#include "playerctrl.h"
#include "taskqueue.h"

class OutgoingPacket;

class GameEngine
{
 public:
  class Token
  {
    Token() = default;
    friend class GameEngine;
  };

  GameEngine(boost::asio::io_service* io_service,
             const std::string& loginMessage,
             const std::string& dataFilename,
             const std::string& itemsFilename,
             const std::string& worldFilename);

  // Not copyable
  GameEngine(const GameEngine&) = delete;
  GameEngine& operator=(const GameEngine&) = delete;

  ~GameEngine();

  bool start();
  bool stop();

  template<class F, class... Args>
  void addTask(F&& f, Args&&... args)
  {
    taskQueue_.addTask(std::bind(f, this, token_, args...));
  }

  CreatureId createPlayer(const std::string& name, const std::function<void(OutgoingPacket&&)>& sendPacket);

  void playerSpawn(Token t, CreatureId creatureId);
  void playerDespawn(Token t, CreatureId creatureId);

  void playerMove(Token t, CreatureId creatureId, Direction direction);
  void playerMovePath(Token t, CreatureId creatureId, const std::deque<Direction>& path);
  void playerMovePathStep(Token t, CreatureId creatureId);  // can be private
  void playerCancelMove(Token t, CreatureId creatureId);
  void playerTurn(Token t, CreatureId creatureId, Direction direction);

  void playerSay(Token t,
                 CreatureId creatureId,
                 uint8_t type,
                 const std::string& message,
                 const std::string& receiver,
                 uint16_t channelId);

  void playerMoveItemFromPosToPos(Token t,
                                  CreatureId creatureId,
                                  const Position& fromPosition,
                                  int fromStackPos,
                                  int itemId,
                                  int count,
                                  const Position& toPosition);
  void playerMoveItemFromPosToInv(Token t,
                                  CreatureId creatureId,
                                  const Position& fromPosition,
                                  int fromStackPos,
                                  int itemId,
                                  int count,
                                  int inventoryId);
  void playerMoveItemFromInvToPos(Token t,
                                  CreatureId creatureId,
                                  int fromInventoryId,
                                  int itemId,
                                  int count,
                                  const Position& toPosition);
  void playerMoveItemFromInvToInv(Token t,
                                  CreatureId creatureId,
                                  int fromInventoryId,
                                  int itemId,
                                  int count,
                                  int toInventoryId);

  void playerUseInvItem(Token t, CreatureId creatureId, int itemId, int inventoryIndex);
  void playerUsePosItem(Token t, CreatureId creatureId, int itemId, const Position& position, int stackPos);

  void playerLookAtInvItem(Token t, CreatureId creatureId, int inventoryIndex, ItemId itemId);
  void playerLookAtPosItem(Token t, CreatureId creatureId, const Position& position, ItemId itemId, int stackPos);

 private:
  // Use these instead of the unordered_maps directly
  Player& getPlayer(CreatureId creatureId) { return players_.at(creatureId); }
  PlayerCtrl& getPlayerCtrl(CreatureId creatureId) { return playerCtrls_.at(creatureId); }

  // Task stuff
  using TaskFunction = std::function<void(void)>;
  void onTask(const TaskFunction& task);

  TaskQueue<TaskFunction> taskQueue_;
  Token token_;

  enum State
  {
    INITIALIZED,
    RUNNING,
    CLOSING,
    CLOSED,
  } state_;

  std::unordered_map<CreatureId, Player> players_;
  std::unordered_map<CreatureId, PlayerCtrl> playerCtrls_;

  std::string loginMessage_;

  std::unique_ptr<World> world_;
};

#endif  // WORLDSERVER_GAMEENGINE_H_
