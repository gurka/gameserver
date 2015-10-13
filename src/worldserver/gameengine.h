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
#include <list>
#include <memory>
#include <string>

#include <boost/asio.hpp>  //NOLINT

#include "world/world.h"
#include "playerctrl.h"
#include "taskqueue.h"

class OutgoingPacket;

class GameEngine
{
 public:
  GameEngine(boost::asio::io_service* io_service, const std::string& loginMessage, const std::string& dataFilename,
             const std::string& worldFilename, const std::string& itemsFilename);
  GameEngine(const GameEngine&) = delete;
  GameEngine& operator=(const GameEngine&) = delete;
  ~GameEngine();

  bool start();
  bool stop();

  CreatureId playerSpawn(const std::string& name, const std::function<void(const OutgoingPacket&)>& sendPacket);
  bool playerDespawn(CreatureId creatureId);
  void playerMove(CreatureId creatureId, Direction direction);
  void playerMove(CreatureId creatureId, const std::list<Direction>& path);
  void playerTurn(CreatureId creatureId, Direction direction);
  void playerSay(CreatureId creatureId, uint8_t type, const std::string& message,
                 const std::string& receiver, uint16_t channelId);
  void playerMoveItem(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                      int itemId, int count, const Position& toPosition);
  void playerMoveItem(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                      int itemId, int count, int inventoryId);
  void playerMoveItem(CreatureId creatureId, int fromInventoryId, int itemId, int count, const Position& toPosition);
  void playerMoveItem(CreatureId creatureId, int fromInventoryId, int itemId, int count, int toInventoryId);
  void playerUseItem(CreatureId creatureId, int itemId, int inventoryIndex);
  void playerUseItem(CreatureId creatureId, int itemId, const Position& position, int stackPos);
  void playerLookAt(CreatureId creatureId, const Position& position, ItemId itemId);

 private:
  // Use these instead of the unordered_maps directly
  Player& getPlayer(CreatureId creatureId) { return *(players_.at(creatureId).get()); }
  PlayerCtrl& getPlayerCtrl(CreatureId creatureId) { return *(playerCtrls_.at(creatureId).get()); }

  using TaskFunction = std::function<void(void)>;
  void onTask(const TaskFunction& task);

  enum State
  {
    INITIALIZED,
    RUNNING,
    CLOSING,
    CLOSED,
  };
  State state_;

  TaskQueue<TaskFunction> taskQueue_;

  std::unordered_map<CreatureId, std::unique_ptr<Player>> players_;
  std::unordered_map<CreatureId, std::unique_ptr<PlayerCtrl>> playerCtrls_;

  std::string loginMessage_;

  World world_;
};

#endif  // WORLDSERVER_GAMEENGINE_H_
