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

#ifndef PROTOCOL_EXPORT_PROTOCOL_HELPER_H_
#define PROTOCOL_EXPORT_PROTOCOL_HELPER_H_

#include <cstdint>
#include <array>
#include <string>

#include "game_position.h"
#include "thing.h"
#include "creature.h"
#include "item.h"
#include "container.h"
#include "protocol_types.h"

namespace gameengine
{

class Equipment;
class Player;

}

namespace network
{

class IncomingPacket;
class OutgoingPacket;

}

namespace common
{

struct Outfit;
class Position;

}

namespace world
{

class Tile;
class World;

}

namespace protocol
{

using KnownCreatures = std::array<common::CreatureId, 64>;
using KnownContainers = std::array<common::ItemUniqueId, 64>;

common::Position getPosition(network::IncomingPacket* packet);
common::Outfit getOutfit(network::IncomingPacket* packet);
common::GamePosition getGamePosition(KnownContainers* container_ids, network::IncomingPacket* packet);
common::ItemPosition getItemPosition(KnownContainers* container_ids, network::IncomingPacket* packet);
Creature getCreature(bool known, network::IncomingPacket* packet);
Item getItem(network::IncomingPacket* packet);

void addPosition(const common::Position& position, network::OutgoingPacket* packet);
void addThing(const common::Thing& thing,
              KnownCreatures* known_creatures,
              network::OutgoingPacket* packet);
void addCreature(const common::Creature* creature,
                 KnownCreatures* known_creatures,
                 network::OutgoingPacket* packet);
void addItem(const common::Item* item, network::OutgoingPacket* packet);
void addMapData(const world::World& world_interface,
                const common::Position& position,
                int width,
                int height,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet);
void addTileData(const world::Tile& tile, KnownCreatures* known_creatures, network::OutgoingPacket* packet);
void addOutfitData(const common::Outfit& outfit, network::OutgoingPacket* packet);

namespace server
{

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

}  // namespace server

namespace client
{

// 0x0A
Login getLogin(network::IncomingPacket* packet);

// 0x14
LoginFailed getLoginFailed(network::IncomingPacket* packet);

// 0x64
Map getMap(int width, int height, network::IncomingPacket* packet);

// 0x83
MagicEffect getMagicEffect(network::IncomingPacket* packet);

// 0x78 0x79
Equipment getEquipment(bool empty, network::IncomingPacket* packet);

// 0x0A
PlayerStats getPlayerStats(network::IncomingPacket* packet);

// 0x82
WorldLight getWorldLight(network::IncomingPacket* packet);

// 0xA1
PlayerSkills getPlayerSkills(network::IncomingPacket* packet);

// 0xB4
TextMessage getTextMessage(network::IncomingPacket* packet);

// 0x6A
ThingAdded getThingAdded(network::IncomingPacket* packet);

// 0x6B
ThingChanged getThingChanged(network::IncomingPacket* packet);

// 0x6C
ThingRemoved getThingRemoved(network::IncomingPacket* packet);

// 0x6D
ThingMoved getThingMoved(network::IncomingPacket* packet);

}  // namespace client

}  // namespace protocol

#endif  // PROTOCOL_EXPORT_PROTOCOL_HELPER_H_
