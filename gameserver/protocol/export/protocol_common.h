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

#ifndef PROTOCOL_EXPORT_PROTOCOL_COMMON_H_
#define PROTOCOL_EXPORT_PROTOCOL_COMMON_H_

#include <array>
#include <variant>
#include <vector>

#include "game_position.h"
#include "creature.h"
#include "item.h"

namespace network
{

class IncomingPacket;
class OutgoingPacket;

}

namespace common
{

struct Outfit;
class Position;
class Thing;

}

namespace world
{

class Tile;
class World;

}

namespace protocol
{

struct Creature
{
  bool known;
  std::uint32_t id_to_remove;  // only if known = false
  std::uint32_t id;
  std::string name;  // only if known = false
  std::uint8_t health_percent;
  common::Direction direction;
  common::Outfit outfit;
  std::uint16_t speed;
};

struct Item
{
  std::uint16_t item_type_id;
  std::uint8_t extra;  // only if type is stackable or multitype
};

using Thing = std::variant<Creature, Item>;

struct Tile
{
  bool skip;
  std::vector<Thing> things;
};

using KnownCreatures = std::array<common::CreatureId, 64>;
using KnownContainers = std::array<common::ItemUniqueId, 64>;

common::Position getPosition(network::IncomingPacket* packet);
common::Outfit getOutfit(network::IncomingPacket* packet);
common::GamePosition getGamePosition(KnownContainers* container_ids, network::IncomingPacket* packet);
common::ItemPosition getItemPosition(KnownContainers* container_ids, network::IncomingPacket* packet);
Creature getCreature(bool known, network::IncomingPacket* packet);
Item getItem(network::IncomingPacket* packet);
Thing getThing(network::IncomingPacket* packet);

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

}  // namespace protocol

#endif  // PROTOCOL_EXPORT_PROTOCOL_COMMON_H_
