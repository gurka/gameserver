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

#ifndef PROTOCOL_EXPORT_PROTOCOL_SERVER_H_
#define PROTOCOL_EXPORT_PROTOCOL_SERVER_H_

#include <cstdint>
#include <deque>
#include <string>

#include "protocol_common.h"

namespace gameengine
{

struct Container;
class Equipment;
class Player;

}  // namespace gameengine

namespace world
{

class Tile;
class World;

}  // namespace world

namespace protocol::server
{

struct Login
{
  std::uint8_t unknown1;
  std::uint8_t client_os;
  std::uint16_t client_version;
  std::uint8_t unknown2;
  std::string character_name;
  std::string password;
};

struct MoveClick
{
  std::deque<common::Direction> path;
};

struct MoveItem  // or MoveThing?
{
  common::ItemPosition from_item_position;
  common::GamePosition to_game_position;
  std::uint8_t count;
};

struct UseItem
{
  common::ItemPosition item_position;
  std::uint8_t new_container_id;
};

struct CloseContainer
{
  std::uint8_t container_id;
};

struct OpenParentContainer
{
  std::uint8_t container_id;
};

struct LookAt
{
  common::ItemPosition item_position;
};

struct Say
{
  std::uint8_t type;
  std::string receiver;
  std::uint16_t channel_id;
  std::string message;
};

// Writing packets

// 0x0A
void addLogin(common::CreatureId player_id, std::uint16_t server_beat, network::OutgoingPacket* packet);

// 0x14
void addLoginFailed(const std::string& reason, network::OutgoingPacket* packet);

// 0x64
void addMapFull(const world::World& world_interface,
                const common::Position& position,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet);

// 0x65, 0x66, 0x67, 0x68
void addMap(const world::World& world_interface,
            const common::Position& old_position,
            const common::Position& new_position,
            KnownCreatures* known_creatures,
            network::OutgoingPacket* packet);

// 0x69
void addTileUpdated(const common::Position& position,
                    const world::World& world_interface,
                    KnownCreatures* known_creatures,
                    network::OutgoingPacket* packet);

// 0x6A
void addThingAdded(const common::Position& position,
                   const common::Thing& thing,
                   KnownCreatures* known_creatures,
                   network::OutgoingPacket* packet);

// 0x6B
void addThingChanged(const common::Position& position,
                     std::uint8_t stackpos,
                     const common::Thing& thing,
                     KnownCreatures* known_creatures,
                     network::OutgoingPacket* packet);

// 0x6C
void addThingRemoved(const common::Position& position, std::uint8_t stackpos, network::OutgoingPacket* packet);

// 0x6D
void addThingMoved(const common::Position& old_position,
                   std::uint8_t old_stackpos,
                   const common::Position& new_position,
                   network::OutgoingPacket* packet);

// 0x6E
void addContainerOpen(std::uint8_t container_id,
                      const common::Thing& thing,
                      const gameengine::Container& container,
                      network::OutgoingPacket* packet);

// 0x6F
void addContainerClose(std::uint8_t container_id, network::OutgoingPacket* packet);

// 0x70
void addContainerAddItem(std::uint8_t container_id, const common::Thing& thing, network::OutgoingPacket* packet);

// 0x71
void addContainerUpdateItem(std::uint8_t container_id,
                            std::uint8_t container_slot,
                            const common::Thing& thing,
                            network::OutgoingPacket* packet);

// 0x72
void addContainerRemoveItem(std::uint8_t container_id, std::uint8_t container_slot, network::OutgoingPacket* packet);

// 0x78, 0x79
void addEquipmentUpdated(const gameengine::Equipment& equipment, std::uint8_t inventory_index, network::OutgoingPacket* packet);

// 0x82
void addWorldLight(std::uint8_t intensity, std::uint8_t color, network::OutgoingPacket* packet);

// 0x83
void addMagicEffect(const common::Position& position, std::uint8_t type, network::OutgoingPacket* packet);

// 0xA0
void addPlayerStats(const gameengine::Player& player, network::OutgoingPacket* packet);

// 0xA1
void addPlayerSkills(const gameengine::Player& player, network::OutgoingPacket* packet);

// 0xAA
void addTalk(const std::string& name,
             std::uint8_t type,
             const common::Position& position,
             const std::string& message,
             network::OutgoingPacket* packet);

// 0xB4
void addTextMessage(std::uint8_t type, const std::string& text, network::OutgoingPacket* packet);

// 0xB5
void addCancelMove(network::OutgoingPacket* packet);

// Helpers
void addMapData(const world::World& world_interface,
                const common::Position& position,
                int width,
                int height,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet);

void addTileData(const world::Tile& tile, KnownCreatures* known_creatures, network::OutgoingPacket* packet);

// Reading packets

// 0x0A
Login getLogin(network::IncomingPacket* packet);

// 0x64
MoveClick getMoveClick(network::IncomingPacket* packet);

// 0x78
MoveItem getMoveItem(KnownContainers* container_ids, network::IncomingPacket* packet);

// 0x82
UseItem getUseItem(KnownContainers* container_ids, network::IncomingPacket* packet);

// 0x87
CloseContainer getCloseContainer(network::IncomingPacket* packet);

// 0x88
OpenParentContainer getOpenParentContainer(network::IncomingPacket* packet);

// 0x8C
LookAt getLookAt(KnownContainers* container_ids, network::IncomingPacket* packet);

// 0x96
Say getSay(network::IncomingPacket* packet);

}  // namespace protocol::server

#endif  // PROTOCOL_EXPORT_PROTOCOL_SERVER_H_
