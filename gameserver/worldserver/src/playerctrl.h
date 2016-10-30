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

#ifndef WORLDSERVER_PLAYERCTRL_H_
#define WORLDSERVER_PLAYERCTRL_H_

#include <deque>
#include <functional>
#include <string>
#include <unordered_set>

#include <boost/date_time/posix_time/posix_time.hpp>  //NOLINT

#include "player.h"
#include "creaturectrl.h"
#include "worldinterface.h"
#include "creature.h"
#include "position.h"
#include "item.h"

class OutgoingPacket;

class PlayerCtrl : public CreatureCtrl
{
 public:
  PlayerCtrl(WorldInterface* worldInterface,
             CreatureId creatureId,
             std::function<void(OutgoingPacket&&)> sendPacket)
    : worldInterface_(worldInterface),
      creatureId_(creatureId),
      sendPacket_(sendPacket),
      nextWalkTime_(boost::posix_time::microsec_clock::local_time())
  {
  }

  // From CreatureCtrl
  void onCreatureSpawn(const Creature& creature, const Position& position);
  void onCreatureDespawn(const Creature& creature, const Position& position, uint8_t stackPos);
  void onCreatureMove(const Creature& creature,
                      const Position& oldPosition, uint8_t oldStackPos,
                      const Position& newPosition, uint8_t newStackPos);
  void onCreatureTurn(const Creature& creature, const Position& position, uint8_t stackPos);
  void onCreatureSay(const Creature& creature, const Position& position, const std::string& message);

  void onItemRemoved(const Position& position, uint8_t stackPos);
  void onItemAdded(const Item& item, const Position& position);

  void onTileUpdate(const Position& position);

  // Player specific ctrl
  void onPlayerSpawn(const Player& player, const Position& position, const std::string& loginMessage);

  void onEquipmentUpdated(const Player& player, int inventoryIndex);

  void onUseItem(const Item& item);

  void sendTextMessage(const std::string& message);
  void sendCancel(const std::string& message);

  void queueMoves(const std::deque<Direction>& moves);
  bool hasQueuedMove() const { return !queuedMoves_.empty(); }
  Direction getNextQueuedMove();
  void cancelMove();

  boost::posix_time::ptime getNextWalkTime() const;

 private:
  bool canSee(const Position& position) const;

  // Packet functions
  void addPosition(const Position& position, OutgoingPacket* packet) const;
  void addMapData(const Position& position, int width, int height, OutgoingPacket* packet);
  void addCreature(const Creature& creature, OutgoingPacket* packet);
  void addItem(const Item& item, OutgoingPacket* packet) const;
  void addEquipment(const Player& player, int inventoryIndex, OutgoingPacket* packet) const;

  WorldInterface* worldInterface_;
  CreatureId creatureId_;
  std::function<void(OutgoingPacket&&)> sendPacket_;

  std::unordered_set<CreatureId> knownCreatures_;

  boost::posix_time::ptime nextWalkTime_;
  std::deque<Direction> queuedMoves_;
};

#endif  // WORLDSERVER_PLAYERCTRL_H_
