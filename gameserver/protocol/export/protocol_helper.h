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

#include "protocol_types.h"
#include "game_position.h"
#include "thing.h"
#include "creature.h"
#include "item.h"
#include "container.h"

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

class Position;
class WorldInterface;
struct Outfit;
class Tile;

namespace protocol_helper
{
using KnownCreatures = std::array<CreatureId, 64>;
using KnownContainers = std::array<ItemUniqueId, 64>;

// Writing packets

// 0x0A
void addLogin(CreatureId player_id, std::uint16_t server_beat, network::OutgoingPacket* packet);

// 0x14
void addLoginFailed(const std::string& reason, network::OutgoingPacket* packet);

// 0x64
void addMapFull(const WorldInterface& world_interface,
                const Position& position,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet);

// 0x65, 0x66, 0x67, 0x68
void addMap(const WorldInterface& world_interface,
            const Position& old_position,
            const Position& new_position,
            KnownCreatures* known_creatures,
            network::OutgoingPacket* packet);

// 0x69
void addTileUpdated(const Position& position,
                    const WorldInterface& world_interface,
                    KnownCreatures* known_creatures,
                    network::OutgoingPacket* packet);

// 0x6A
void addThingAdded(const Position& position,
                   const Thing& thing,
                   KnownCreatures* known_creatures,
                   network::OutgoingPacket* packet);

// 0x6B
void addThingChanged(const Position& position,
                     std::uint8_t stackpos,
                     const Thing& thing,
                     KnownCreatures* known_creatures,
                     network::OutgoingPacket* packet);

// 0x6C
void addThingRemoved(const Position& position, std::uint8_t stackpos, network::OutgoingPacket* packet);

// 0x6D
void addThingMoved(const Position& old_position,
                   std::uint8_t old_stackpos,
                   const Position& new_position,
                   network::OutgoingPacket* packet);

// 0x6E
void addContainerOpen(std::uint8_t container_id,
                      const Thing& thing,
                      const gameengine::Container& container,
                      network::OutgoingPacket* packet);

// 0x6F
void addContainerClose(std::uint8_t container_id, network::OutgoingPacket* packet);

// 0x70
void addContainerAddItem(std::uint8_t container_id, const Thing& thing, network::OutgoingPacket* packet);

// 0x71
void addContainerUpdateItem(std::uint8_t container_id,
                            std::uint8_t container_slot,
                            const Thing& thing,
                            network::OutgoingPacket* packet);

// 0x72
void addContainerRemoveItem(std::uint8_t container_id, std::uint8_t container_slot, network::OutgoingPacket* packet);

// 0x78, 0x79
void addEquipmentUpdated(const gameengine::Equipment& equipment, std::uint8_t inventory_index, network::OutgoingPacket* packet);

// 0x82
void addWorldLight(std::uint8_t intensity, std::uint8_t color, network::OutgoingPacket* packet);

// 0x83
void addMagicEffect(const Position& position, std::uint8_t type, network::OutgoingPacket* packet);

// 0xA0
void addPlayerStats(const gameengine::Player& player, network::OutgoingPacket* packet);

// 0xA1
void addPlayerSkills(const gameengine::Player& player, network::OutgoingPacket* packet);

// 0xAA
void addTalk(const std::string& name,
             std::uint8_t type,
             const Position& position,
             const std::string& message,
             network::OutgoingPacket* packet);

// 0xB4
void addTextMessage(std::uint8_t type, const std::string& text, network::OutgoingPacket* packet);

// 0xB5
void addCancelMove(network::OutgoingPacket* packet);

// Writing helpers
void addPosition(const Position& position, network::OutgoingPacket* packet);
void addThing(const Thing& thing,
              KnownCreatures* known_creatures,
              network::OutgoingPacket* packet);
void addCreature(const Creature* creature,
                 KnownCreatures* known_creatures,
                 network::OutgoingPacket* packet);
void addItem(const Item* item, network::OutgoingPacket* packet);
void addMapData(const WorldInterface& world_interface,
                const Position& position,
                int width,
                int height,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet);
void addTileData(const Tile& tile, KnownCreatures* known_creatures, network::OutgoingPacket* packet);
void addOutfitData(const Outfit& outfit, network::OutgoingPacket* packet);


// Reading packets
protocol_types::Login getLogin(network::IncomingPacket* packet);
protocol_types::MoveClick getMoveClick(network::IncomingPacket* packet);
protocol_types::MoveItem getMoveItem(KnownContainers* container_ids, network::IncomingPacket* packet);
protocol_types::UseItem getUseItem(KnownContainers* container_ids, network::IncomingPacket* packet);
protocol_types::CloseContainer getCloseContainer(network::IncomingPacket* packet);
protocol_types::OpenParentContainer getOpenParentContainer(network::IncomingPacket* packet);
protocol_types::LookAt getLookAt(KnownContainers* container_ids, network::IncomingPacket* packet);
protocol_types::Say getSay(network::IncomingPacket* packet);

// Reading helpers
Position getPosition(network::IncomingPacket* packet);
Outfit getOutfit(network::IncomingPacket* packet);
gameengine::GamePosition getGamePosition(KnownContainers* container_ids, network::IncomingPacket* packet);
gameengine::ItemPosition getItemPosition(KnownContainers* container_ids, network::IncomingPacket* packet);

}  // namespace protocol_helper

#endif  // PROTOCOL_EXPORT_PROTOCOL_HELPER_H_
