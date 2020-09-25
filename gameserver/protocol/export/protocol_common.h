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
#include "data_loader.h"

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

namespace protocol
{

struct Creature
{
  enum class Update : std::uint16_t
  {
    NEW       = 0x0061,
    FULL      = 0x0062,
    DIRECTION = 0x0063,
  };

  Update update;
  std::uint32_t id_to_remove;   // only if NEW
  std::uint32_t id;             // always
  std::string name;             // only if NEW
  std::uint8_t health_percent;  // only if NEW or FULL
  common::Direction direction;  // always
  common::Outfit outfit;        // only if NEW or FULL
  std::uint16_t speed;          // only if NEW or FULL
};

struct Item
{
  std::uint16_t item_type_id;
  std::uint8_t extra;  // only if is_stackable, is_fluid_container or is_splash
};

using Thing = std::variant<Creature, Item>;

struct Tile
{
  bool skip;
  std::vector<Thing> things;
};

using KnownCreatures = std::array<common::CreatureId, 64>;
using KnownContainers = std::array<common::ItemUniqueId, 64>;

void setItemTypes(const utils::data_loader::ItemTypes* item_types_in);

common::Position getPosition(network::IncomingPacket* packet);
common::Outfit getOutfit(network::IncomingPacket* packet);
common::GamePosition getGamePosition(KnownContainers* container_ids, network::IncomingPacket* packet);
common::ItemPosition getItemPosition(KnownContainers* container_ids, network::IncomingPacket* packet);
Creature getCreature(Creature::Update update, network::IncomingPacket* packet);
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
void addOutfitData(const common::Outfit& outfit, network::OutgoingPacket* packet);

}  // namespace protocol

#endif  // PROTOCOL_EXPORT_PROTOCOL_COMMON_H_
