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

#ifndef WORLDSERVER_SRC_PROTOCOL_HELPER_H_
#define WORLDSERVER_SRC_PROTOCOL_HELPER_H_

#include <cstdint>
#include <array>
#include <string>

#include "game_position.h"
#include "creature.h"
#include "item.h"

class Position;
class OutgoingPacket;
class WorldInterface;
class Equipment;
class Player;
class IncomingPacket;

namespace ProtocolHelper
{
  // Helper functions for creating OutgoingPackets
  bool canSee(const Position& player_position, const Position& to_position);
  void addPosition(const Position& position, OutgoingPacket* packet);
  void addFullMapData(const WorldInterface& world_interface,
                      const Position& position,
                      std::array<CreatureId, 64>* knownCreatures,
                      OutgoingPacket* packet);
  void addMapData(const WorldInterface& world_interface,
                  const Position& position,
                  int width,
                  int height,
                  std::array<CreatureId, 64>* knownCreatures,
                  OutgoingPacket* packet);
  void addCreature(const Creature& creature,
                   std::array<CreatureId, 64>* knownCreatures,
                   OutgoingPacket* packet);
  void addItem(const Item& item, OutgoingPacket* packet);
  void addEquipment(const Equipment& equipment, int inventoryIndex, OutgoingPacket* packet);
  void addMagicEffect(const Position& position, std::uint8_t type, OutgoingPacket* packet);
  void addPlayerStats(const Player& player, OutgoingPacket* packet);
  void addPlayerSkills(const Player& player, OutgoingPacket* packet);
  void addWorldLight(std::uint8_t intensity, std::uint8_t color, OutgoingPacket* packet);

  // Helper functions for parsing IncomingPackets
  GamePosition getGamePosition(std::array<ItemUniqueId, 64>* containerIds, IncomingPacket* packet);
  ItemPosition getItemPosition(std::array<ItemUniqueId, 64>* containerIds, IncomingPacket* packet);
}

#endif  // WORLDSERVER_SRC_PROTOCOL_HELPER_H_
