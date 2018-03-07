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

#ifndef WORLDSERVER_PLAYERMANAGER_H_
#define WORLDSERVER_PLAYERMANAGER_H_

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "world_task_queue.h"
#include "player.h"
#include "position.h"
#include "container_manager.h"

class OutgoingPacket;
class PlayerCtrl;
class World;

class PlayerManager
{
 public:
  PlayerManager(WorldTaskQueue* worldTaskQueue, std::string loginMessage);

  // Delete copy constructors
  PlayerManager(const PlayerManager&) = delete;
  PlayerManager& operator=(const PlayerManager&) = delete;

  void spawn(const std::string& name, PlayerCtrl* player_ctrl);
  void despawn(CreatureId creatureId);

  void move(CreatureId creatureId, Direction direction);
  void movePath(CreatureId creatureId, const std::deque<Direction>& path);
  void cancelMove(CreatureId creatureId);
  void turn(CreatureId creatureId, Direction direction);

  void say(CreatureId creatureId, uint8_t type, const std::string& message, const std::string& receiver, uint16_t channelId);

  // TODO(gurka): Maybe name them all 'moveItem'? Would be OK since Position can't be casted to int
  void moveItemFromPosToPos(CreatureId creatureId, const Position& fromPosition, int fromStackPos, int itemId, int count, const Position& toPosition);
  void moveItemFromPosToInv(CreatureId creatureId, const Position& fromPosition, int fromStackPos, int itemId, int count, int inventoryId);
  void moveItemFromInvToPos(CreatureId creatureId, int fromInventoryId, int itemId, int count, const Position& toPosition);
  void moveItemFromInvToInv(CreatureId creatureId, int fromInventoryId, int itemId, int count, int toInventoryId);

  // TODO(gurka): Same as above
  void useInvItem(CreatureId creatureId, int itemId, int inventoryIndex);
  void usePosItem(CreatureId creatureId, int itemId, const Position& position, int stackPos);

  // TODO(gurka): Same as above
  void lookAtInvItem(CreatureId creatureId, int inventoryIndex, ItemId itemId);
  void lookAtPosItem(CreatureId creatureId, const Position& position, ItemId itemId, int stackPos);

 private:
  // Use these instead of the unordered_maps directly
  Player& getPlayer(CreatureId creatureId) { return playerPlayerCtrl_.at(creatureId).player; }
  PlayerCtrl* getPlayerCtrl(CreatureId creatureId) { return playerPlayerCtrl_.at(creatureId).player_ctrl; }

  // Player + PlayerCtrl
  struct PlayerPlayerCtrl
  {
    Player player;
    PlayerCtrl* player_ctrl;
  };
  std::unordered_map<CreatureId, PlayerPlayerCtrl> playerPlayerCtrl_;

  WorldTaskQueue* worldTaskQueue_;
  std::string loginMessage_;
  ContainerManager containerManager_;
};

#endif  // WORLDSERVER_PLAYERMANAGER_H_
