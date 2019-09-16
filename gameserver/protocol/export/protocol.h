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

namespace world
{
struct Outfit;
class Position;
class Tile;
class World;
}

namespace protocol
{

// Common stuff
world::Position getPosition(network::IncomingPacket* packet);
world::Outfit getOutfit(network::IncomingPacket* packet);

namespace server
{

using KnownCreatures = std::array<world::CreatureId, 64>;
using KnownContainers = std::array<world::ItemUniqueId, 64>;

// Writing packets

// 0x0A
void addLogin(world::CreatureId player_id, std::uint16_t server_beat, network::OutgoingPacket* packet);

// 0x14
void addLoginFailed(const std::string& reason, network::OutgoingPacket* packet);

// 0x64
void addMapFull(const world::World& world_interface,
                const world::Position& position,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet);

// 0x65, 0x66, 0x67, 0x68
void addMap(const world::World& world_interface,
            const world::Position& old_position,
            const world::Position& new_position,
            KnownCreatures* known_creatures,
            network::OutgoingPacket* packet);

// 0x69
void addTileUpdated(const world::Position& position,
                    const world::World& world_interface,
                    KnownCreatures* known_creatures,
                    network::OutgoingPacket* packet);

// 0x6A
void addThingAdded(const world::Position& position,
                   const world::Thing& thing,
                   KnownCreatures* known_creatures,
                   network::OutgoingPacket* packet);

// 0x6B
void addThingChanged(const world::Position& position,
                     std::uint8_t stackpos,
                     const world::Thing& thing,
                     KnownCreatures* known_creatures,
                     network::OutgoingPacket* packet);

// 0x6C
void addThingRemoved(const world::Position& position, std::uint8_t stackpos, network::OutgoingPacket* packet);

// 0x6D
void addThingMoved(const world::Position& old_position,
                   std::uint8_t old_stackpos,
                   const world::Position& new_position,
                   network::OutgoingPacket* packet);

// 0x6E
void addContainerOpen(std::uint8_t container_id,
                      const world::Thing& thing,
                      const gameengine::Container& container,
                      network::OutgoingPacket* packet);

// 0x6F
void addContainerClose(std::uint8_t container_id, network::OutgoingPacket* packet);

// 0x70
void addContainerAddItem(std::uint8_t container_id, const world::Thing& thing, network::OutgoingPacket* packet);

// 0x71
void addContainerUpdateItem(std::uint8_t container_id,
                            std::uint8_t container_slot,
                            const world::Thing& thing,
                            network::OutgoingPacket* packet);

// 0x72
void addContainerRemoveItem(std::uint8_t container_id, std::uint8_t container_slot, network::OutgoingPacket* packet);

// 0x78, 0x79
void addEquipmentUpdated(const gameengine::Equipment& equipment, std::uint8_t inventory_index, network::OutgoingPacket* packet);

// 0x82
void addWorldLight(std::uint8_t intensity, std::uint8_t color, network::OutgoingPacket* packet);

// 0x83
void addMagicEffect(const world::Position& position, std::uint8_t type, network::OutgoingPacket* packet);

// 0xA0
void addPlayerStats(const gameengine::Player& player, network::OutgoingPacket* packet);

// 0xA1
void addPlayerSkills(const gameengine::Player& player, network::OutgoingPacket* packet);

// 0xAA
void addTalk(const std::string& name,
             std::uint8_t type,
             const world::Position& position,
             const std::string& message,
             network::OutgoingPacket* packet);

// 0xB4
void addTextMessage(std::uint8_t type, const std::string& text, network::OutgoingPacket* packet);

// 0xB5
void addCancelMove(network::OutgoingPacket* packet);

// Writing helpers
void addPosition(const world::Position& position, network::OutgoingPacket* packet);
void addThing(const world::Thing& thing,
              KnownCreatures* known_creatures,
              network::OutgoingPacket* packet);
void addCreature(const world::Creature* creature,
                 KnownCreatures* known_creatures,
                 network::OutgoingPacket* packet);
void addItem(const world::Item* item, network::OutgoingPacket* packet);
void addMapData(const world::World& world_interface,
                const world::Position& position,
                int width,
                int height,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet);
void addTileData(const world::Tile& tile, KnownCreatures* known_creatures, network::OutgoingPacket* packet);
void addOutfitData(const world::Outfit& outfit, network::OutgoingPacket* packet);


// Reading packets
Login getLogin(network::IncomingPacket* packet);
MoveClick getMoveClick(network::IncomingPacket* packet);
MoveItem getMoveItem(KnownContainers* container_ids, network::IncomingPacket* packet);
UseItem getUseItem(KnownContainers* container_ids, network::IncomingPacket* packet);
CloseContainer getCloseContainer(network::IncomingPacket* packet);
OpenParentContainer getOpenParentContainer(network::IncomingPacket* packet);
LookAt getLookAt(KnownContainers* container_ids, network::IncomingPacket* packet);
Say getSay(network::IncomingPacket* packet);

// Reading helpers
gameengine::GamePosition getGamePosition(KnownContainers* container_ids, network::IncomingPacket* packet);
gameengine::ItemPosition getItemPosition(KnownContainers* container_ids, network::IncomingPacket* packet);

}  // namespace server

namespace client
{

client::Login getLogin(network::IncomingPacket* packet);
client::LoginFailed getLoginFailed(network::IncomingPacket* packet);
client::Creature getCreature(bool known, network::IncomingPacket* packet);
client::Item getItem(network::IncomingPacket* packet);
client::Equipment getEquipment(bool empty, network::IncomingPacket* packet);
client::MagicEffect getMagicEffect(network::IncomingPacket* packet);
client::PlayerStats getPlayerStats(network::IncomingPacket* packet);
client::WorldLight getWorldLight(network::IncomingPacket* packet);
client::PlayerSkills getPlayerSkills(network::IncomingPacket* packet);
client::TextMessage getTextMessage(network::IncomingPacket* packet);
client::MapData getMapData(int width, int height, network::IncomingPacket* packet);
client::CreatureMove getCreatureMove(bool can_see_old_pos,
                                     bool can_see_new_pos,
                                     network::IncomingPacket* packet);

}  // namespace client

}  // namespace protocol

#endif  // PROTOCOL_EXPORT_PROTOCOL_HELPER_H_
