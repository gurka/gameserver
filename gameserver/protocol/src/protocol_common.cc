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
#include "protocol_common.h"

#include <variant>

#include "logger.h"
#include "thing.h"
#include "creature.h"
#include "item.h"
#include "world.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"

namespace protocol
{

common::Position getPosition(network::IncomingPacket* packet)
{
  const auto x  = packet->getU16();
  const auto y  = packet->getU16();
  const auto z  = packet->getU8();
  return { x, y, z };
}

common::Outfit getOutfit(network::IncomingPacket* packet)
{
  common::Outfit outfit;
  packet->get(&outfit.type);
  packet->get(&outfit.head);
  packet->get(&outfit.body);
  packet->get(&outfit.legs);
  packet->get(&outfit.feet);
  return outfit;
}

common::GamePosition getGamePosition(KnownContainers* container_ids, network::IncomingPacket* packet)
{
  const auto x = packet->getU16();
  const auto y = packet->getU16();
  const auto z = packet->getU8();

  LOG_DEBUG("%s: x = 0x%04X, y = 0x%04X, z = 0x%02X", __func__, x, y, z);

  if (x != 0xFFFF)
  {
    // Positions have x not fully set
    return common::GamePosition(common::Position(x, y, z));
  }

  if ((y & 0x40) == 0x00)
  {
    // Inventory have x fully set and 7th bit in y not set
    // Inventory slot is 4 lower bits in y
    return common::GamePosition(y & ~0x40);
  }

  // Container have x fully set and 7th bit in y set
  // Container id is lower 6 bits in y
  // Container slot is z
  const auto container_id = y & ~0x40;
  if (container_id < 0 || container_id > 64)
  {
    LOG_ERROR("%s: invalid container_id: %d", __func__, container_id);
    return {};
  }

  const auto item_unique_id = container_ids->at(container_id);
  if (item_unique_id == common::Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: container_id does not map to a valid ItemUniqueId: %d", __func__, container_id);
    return {};
  }

  return { item_unique_id, z };
}

common::ItemPosition getItemPosition(KnownContainers* container_ids, network::IncomingPacket* packet)
{
  const auto game_position = getGamePosition(container_ids, packet);
  const auto item_id = packet->getU16();
  const auto stackpos = packet->getU8();

  return { game_position, item_id, stackpos };
}

Creature getCreature(bool known, network::IncomingPacket* packet)
{
  Creature creature;
  creature.known = known;
  if (creature.known)
  {
    packet->get(&creature.id);
  }
  else
  {
    packet->get(&creature.id_to_remove);
    packet->get(&creature.id);
    packet->get(&creature.name);
  }
  packet->get(&creature.health_percent);
  creature.direction = static_cast<common::Direction>(packet->getU8());
  creature.outfit = getOutfit(packet);
  packet->getU16();  // unknown 0xDC00
  packet->get(&creature.speed);
  return creature;
}

Item getItem(network::IncomingPacket* packet)
{
  Item item;
  packet->get(&item.item_type_id);
  // Need to make ItemType available to be able to check if we should
  // read extra or not. For now assume not to read it
  return item;
}

Thing getThing(network::IncomingPacket* packet)
{
  if (packet->peekU16() == 0x0061 || packet->peekU16() == 0x0062)
  {
    return protocol::getCreature(packet->getU16() == 0x0062, packet);
  }
  else
  {
    return protocol::getItem(packet);
  }
}

void addPosition(const common::Position& position, network::OutgoingPacket* packet)
{
  packet->add(position.getX());
  packet->add(position.getY());
  packet->add(position.getZ());
}

void addThing(const common::Thing& thing, KnownCreatures* known_creatures, network::OutgoingPacket* packet)
{
  if (thing.hasCreature())
  {
    addCreature(thing.creature(), known_creatures, packet);
  }
  else  // const common::Item*
  {
    addItem(thing.item(), packet);
  }
}

void addCreature(const common::Creature* creature, KnownCreatures* known_creatures, network::OutgoingPacket* packet)
{
  // First check if we know about this creature or not
  auto it = std::find(known_creatures->begin(), known_creatures->end(), creature->getCreatureId());
  if (it == known_creatures->end())
  {
    // Find an empty spot
    auto unused = std::find(known_creatures->begin(), known_creatures->end(), common::Creature::INVALID_ID);
    if (unused == known_creatures->end())
    {
      // No empty spot!
      // TODO(simon): Figure out how to handle this - related to "creatureId to remove" below?
      LOG_ERROR("%s: known_creatures_ is full!", __func__);
      return;
    }

    *unused = creature->getCreatureId();

    packet->addU16(0x0061);  // UnknownCreature
    packet->addU32(0x00);  // creatureId to remove (0x00 = none)
    packet->add(creature->getCreatureId());
    packet->add(creature->getName());
  }
  else
  {
    // Client already know about this creature
    packet->addU16(0x0062);  // OutdatedCreature
    packet->add(creature->getCreatureId());
  }
  // TODO(simon): handle 0x0063 // Creature (+ direction)

  // TODO(simon): This is only for 0x0061 and 0x0062...
  packet->addU8(creature->getHealth() / creature->getMaxHealth() * 100);
  packet->add(static_cast<std::uint8_t>(creature->getDirection()));
  addOutfitData(creature->getOutfit(), packet);

  packet->addU8(0x00);
  packet->addU8(0xDC);

  packet->add(creature->getSpeed());
}

void addItem(const common::Item* item, network::OutgoingPacket* packet)
{
  packet->add(item->getItemTypeId());
  if (item->getItemType().is_stackable)
  {
    packet->add(item->getCount());
  }
  else if (item->getItemType().is_multitype)
  {
    // TODO(simon): getSubType???
    packet->addU8(0);
  }
}

void addMapData(const world::World& world,
                const common::Position& position,
                int width,
                int height,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet)
{
  // Calculate how to iterate over z
  // Valid z is 0..15, 0 is highest and 15 is lowest. 7 is sea level.
  // If on ground or higher (z <= 7) then go over everything above ground (from 7 to 0)
  // If underground (z > 7) then go from two below to two above, with cap on lowest level (from e.g. 8 to 12, if z = 10)
  const auto z_start = position.getZ() > 7 ? (position.getZ() - 2) : 7;
  const auto z_end = position.getZ() > 7 ? std::min(position.getZ() + 2, 15) : 0;
  const auto z_dir = z_start > z_end ? -1 : 1;

  // After sending each tile we should send 0xYY 0xFF where YY is the number of following tiles
  // that are empty and should be skipped. If there are no empty following tiles then we need
  // to send 0x00 0xFF which denotes that this tiles is done.
  // We don't know if the next tile is empty until the next iteration, so we will never send
  // the "this tile is done" bytes on the same iteration as the actual tile, but rather in a
  // later iteration, which is a bit confusing. We start off with -1 so that we don't start the
  // message with saying that a tile is done.
  int skip = -1;

  for (auto z = z_start; z != z_end + z_dir; z += z_dir)
  {
    // Currently we are always on z = 7, so we should send z=7, z=6, ..., z=0
    // But we skip z=6, ..., z=0 as we only have ground
    if (z != 7)
    {
      if (skip != -1)
      {
        // Send current skip value first
        packet->addU8(skip);
        packet->addU8(0xFF);
      }

      // Skip this level (skip width * height tiles)
      packet->addU8(width * height);
      packet->addU8(0xFF);
      skip = -1;
      continue;
    }

    for (auto x = position.getX(); x < position.getX() + width; x++)
    {
      for (auto y = position.getY(); y < position.getY() + height; y++)
      {
        const auto* tile = world.getTile(common::Position(x, y, position.getZ()));
        if (!tile)
        {
          skip += 1;
          if (skip == 0xFF)
          {
            packet->addU8(skip);
            packet->addU8(0xFF);

            // If there is a tile on the next iteration we don't want to send
            // "tile is done", as we just sent one due to skip being max
            skip = -1;
          }
        }
        else
        {
          // Send "tile is done" with the number of tiles that were empty, unless this
          // is the first tile (-1)
          if (skip != -1)
          {
            packet->addU8(skip);
            packet->addU8(0xFF);
          }
          else
          {
            // Don't let skip be -1 more than one iteration
            skip = 0;
          }

          addTileData(*tile, known_creatures, packet);
        }
      }
    }
  }

  // Send last skip value
  if (skip != -1)
  {
    packet->addU8(skip);
    packet->addU8(0xFF);
  }
}

void addTileData(const world::Tile& tile, KnownCreatures* known_creatures, network::OutgoingPacket* packet)
{
  auto count = 0;
  for (const auto& thing : tile.getThings())
  {
    if (count >= 10)
    {
      break;
    }

    addThing(thing, known_creatures, packet);
    count += 1;
  }
}

void addOutfitData(const common::Outfit& outfit, network::OutgoingPacket* packet)
{
  packet->add(outfit.type);
  packet->add(outfit.head);
  packet->add(outfit.body);
  packet->add(outfit.legs);
  packet->add(outfit.feet);
}

}  // namespace protocol
