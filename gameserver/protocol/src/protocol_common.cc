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

#include <algorithm>
#include <variant>

#include "logger.h"
#include "thing.h"
#include "creature.h"
#include "item.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"

namespace protocol
{

static const utils::data_loader::ItemTypes* item_types = nullptr;

void setItemTypes(const utils::data_loader::ItemTypes* item_types_in)
{
  item_types = item_types_in;
}

common::Position getPosition(network::IncomingPacket* packet)
{
  if (packet->peekU16() == 0xFFFFU)
  {
    LOG_ERROR("%s: x=0xFFFF, next 4 bytes might be a CreatureId so we need to implement getKnownThing", __func__);
  }
  const auto x  = packet->getU16();
  const auto y  = packet->getU16();
  const auto z  = packet->getU8();
  return { x, y, z };
}

common::Outfit getOutfit(network::IncomingPacket* packet)
{
  common::Outfit outfit;
  packet->get(&outfit.type);
  if (outfit.type == 0U)
  {
    packet->get(&outfit.item_id);
  }
  else
  {
    packet->get(&outfit.head);
    packet->get(&outfit.body);
    packet->get(&outfit.legs);
    packet->get(&outfit.feet);
  }
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

Creature getCreature(Creature::Update update, network::IncomingPacket* packet)
{
  Creature creature;
  creature.update = update;

  if (update == Creature::Update::DIRECTION)
  {
    packet->get(&creature.id);
    creature.direction = static_cast<common::Direction>(packet->getU8());
  }
  else
  {
    if (update == Creature::Update::FULL)
    {
      packet->get(&creature.id);
    }
    else  // NEW
    {
      packet->get(&creature.id_to_remove);
      packet->get(&creature.id);
      packet->get(&creature.name);
    }

    packet->get(&creature.health_percent);
    creature.direction = static_cast<common::Direction>(packet->getU8());
    creature.outfit = getOutfit(packet);
    packet->getU16();  // light
    packet->get(&creature.speed);

    // TODO(simon): skull and flag
    packet->getU8();
    packet->getU8();
  }

  return creature;
}

Item getItem(network::IncomingPacket* packet)
{
  Item item;
  packet->get(&item.item_type_id);
  const auto& item_type = (*item_types)[item.item_type_id];
  if (item_type.is_stackable || item_type.is_fluid_container || item_type.is_splash)
  {
    packet->get(&item.extra);
  }
  return item;
}

Thing getThing(network::IncomingPacket* packet)
{
  const auto peek = packet->peekU16();
  if (peek == 0x0061U || peek == 0x0062U || peek == 0x0063U)
  {
    return protocol::getCreature(static_cast<Creature::Update>(packet->getU16()), packet);
  }

  return protocol::getItem(packet);
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
  else if (item->getItemType().is_splash)
  {
    // TODO(simon): getSubType???
    packet->addU8(0);
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
