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
#include "protocol_helper.h"

#include <algorithm>

#include "logger.h"
#include "position.h"
#include "outgoing_packet.h"
#include "world_interface.h"
#include "creature.h"
#include "item.h"
#include "player.h"
#include "incoming_packet.h"
#include "tile.h"
#include "protocol_types.h"

namespace protocol_helper
{

void addLogin(CreatureId player_id, std::uint16_t server_beat, OutgoingPacket* packet)
{
  packet->addU8(0x0A);
  packet->add(player_id);
  packet->add(server_beat);
}

void addLoginFailed(const std::string& reason, OutgoingPacket* packet)
{
  packet->addU8(0x14);
  packet->add(reason);
}
void addMapFull(const WorldInterface& world_interface,
                const Position& position,
                KnownCreatures* known_creatures,
                OutgoingPacket* packet)
{
  packet->addU8(0x64);
  addPosition(position, packet);
  addMapData(world_interface,
             Position(position.getX() - 8, position.getY() - 6, position.getZ()),
             18,
             14,
             known_creatures,
             packet);
}

void addMap(const WorldInterface& world_interface,
            const Position& old_position,
            const Position& new_position,
            KnownCreatures* known_creatures,
            OutgoingPacket* packet)
{
  if (old_position.getY() > new_position.getY())
  {
    // North
    packet->addU8(0x65);
    addMapData(world_interface,
               Position(old_position.getX() - 8, new_position.getY() - 6, old_position.getZ()),
               18,
               1,
               known_creatures,
               packet);
  }
  else if (old_position.getY() < new_position.getY())
  {
    // South
    packet->addU8(0x67);
    addMapData(world_interface,
               Position(old_position.getX() - 8, new_position.getY() + 7, old_position.getZ()),
               18,
               1,
               known_creatures,
               packet);
  }

  if (old_position.getX() > new_position.getX())
  {
    // West
    packet->addU8(0x68);
    addMapData(world_interface,
               Position(new_position.getX() - 8, new_position.getY() - 6, old_position.getZ()),
               1,
               14,
               known_creatures,
               packet);
  }
  else if (old_position.getX() < new_position.getX())
  {
    // East
    packet->addU8(0x66);
    addMapData(world_interface,
               Position(new_position.getX() + 9, new_position.getY() - 6, old_position.getZ()),
               1,
               14,
               known_creatures,
               packet);
  }
}

void addTileUpdated(const Position& position,
                    const WorldInterface& world_interface,
                    KnownCreatures* known_creatures,
                    OutgoingPacket* packet)
{
  packet->addU8(0x69);
  addPosition(position, packet);
  const auto* tile = world_interface.getTile(position);
  if (tile)
  {
    addTileData(*tile, known_creatures, packet);
    packet->addU8(0x0);
  }
  else
  {
    packet->addU8(0x01);
  }
  packet->addU8(0xFF);
}

void addThingAdded(const Position& position,
                   const Thing& thing,
                   KnownCreatures* known_creatures,
                   OutgoingPacket* packet)
{
  packet->addU8(0x6A);
  addPosition(position, packet);
  addThing(thing, known_creatures, packet);
}

void addThingChanged(const Position& position,
                     std::uint8_t stackpos,
                     const Thing& thing,
                     KnownCreatures* known_creatures,
                     OutgoingPacket* packet)
{
  packet->addU8(0x6B);
  addPosition(position, packet);
  packet->add(stackpos);
  addThing(thing, known_creatures, packet);

  // Only for creature 0x0063?
  if (thing.creature())
  {
    packet->add(static_cast<std::uint8_t>(thing.creature()->getDirection()));
  }
}

void addThingRemoved(const Position& position, std::uint8_t stackpos, OutgoingPacket* packet)
{
  packet->addU8(0x6C);
  addPosition(position, packet);
  packet->add(stackpos);
}

void addThingMoved(const Position& old_position,
                   std::uint8_t old_stackpos,
                   const Position& new_position,
                   OutgoingPacket* packet)
{
  packet->addU8(0x6D);
  addPosition(old_position, packet);
  packet->add(old_stackpos);
  addPosition(new_position, packet);
}

void addContainerOpen(std::uint8_t container_id,
                      const Thing& thing,
                      const Container& container,
                      OutgoingPacket* packet)
{
  packet->addU8(0x6E);
  packet->add(container_id);
  addThing(thing, nullptr, packet);
  packet->add(thing.item()->getItemType().name);
  packet->add(thing.item()->getItemType().maxitems);
  packet->addU8(container.parent_item_unique_id == Item::INVALID_UNIQUE_ID ? 0x00 : 0x01);
  packet->addU8(container.items.size());
  for (const auto* item : container.items)
  {
    packet->add(item->getItemTypeId());
    if (item->getItemType().is_stackable)  // or splash or fluid container?
    {
      packet->add(item->getCount());
    }
  }
}

void addContainerClose(std::uint8_t container_id, OutgoingPacket* packet)
{
  packet->addU8(0x6F);
  packet->add(container_id);
}

void addContainerAddItem(std::uint8_t container_id, const Thing& thing, OutgoingPacket* packet)
{
  packet->addU8(0x70);
  packet->add(container_id);
  addThing(thing, nullptr, packet);
}

void addContainerUpdateItem(std::uint8_t container_id,
                            std::uint8_t container_slot,
                            const Thing& thing,
                            OutgoingPacket* packet)
{
  packet->addU8(0x71);
  packet->add(container_id);
  packet->add(container_slot);
  addThing(thing, nullptr, packet);
}

void addContainerRemoveItem(std::uint8_t container_id,
                            std::uint8_t container_slot,
                            OutgoingPacket* packet)
{
  packet->addU8(0x72);
  packet->add(container_id);
  packet->add(container_slot);
}

void addEquipmentUpdated(const Equipment& equipment, std::uint8_t inventory_index, OutgoingPacket* packet)
{
  const auto* item = equipment.getItem(inventory_index);
  if (item)
  {
    packet->addU8(0x78);
    packet->add(inventory_index);
    addThing(Thing(item), nullptr, packet);
  }
  else
  {
    packet->addU8(0x79);  // No Item in this slot
    packet->add(inventory_index);
  }
}

void addWorldLight(std::uint8_t intensity, std::uint8_t color, OutgoingPacket* packet)
{
  packet->addU8(0x82);
  packet->add(intensity);
  packet->add(color);
}

void addMagicEffect(const Position& position, std::uint8_t type, OutgoingPacket* packet)
{
  packet->addU8(0x83);
  addPosition(position, packet);
  packet->add(type);
}

void addPlayerStats(const Player& player, OutgoingPacket* packet)
{
  packet->addU8(0xA0);
  packet->add(player.getHealth());
  packet->add(player.getMaxHealth());
  packet->add(player.getCapacity());
  packet->add(player.getExperience());
  packet->add(player.getLevel());
  packet->add(player.getMana());
  packet->add(player.getMaxMana());
  packet->add(player.getMagicLevel());
}

void addPlayerSkills(const Player& player, OutgoingPacket* packet)
{
  // TODO(simon): get skills from Player
  (void)player;

  packet->addU8(0xA1);
  for (auto i = 0; i < 7; i++)
  {
    packet->addU8(10);
  }
}

void addTalk(const std::string& name,
             std::uint8_t type,
             const Position& position,
             const std::string& message,
             OutgoingPacket* packet)
{
  packet->addU8(0xAA);
  packet->add(name);
  packet->add(type);

  // TODO(simon): add full support
  if (type < 4)
  {
    addPosition(position, packet);
  }
  packet->add(message);
}

void addTextMessage(std::uint8_t type, const std::string& text, OutgoingPacket* packet)
{
  packet->addU8(0xB4);
  packet->add(type);
  packet->add(text);
}

void addCancelMove(OutgoingPacket* packet)
{
  packet->addU8(0xB5);
}

void addPosition(const Position& position, OutgoingPacket* packet)
{
  packet->add(position.getX());
  packet->add(position.getY());
  packet->add(position.getZ());
}

void addThing(const Thing& thing,
              KnownCreatures* known_creatures,
              OutgoingPacket* packet)
{
  const auto creature_func = [&packet, &known_creatures](const Creature* creature)
  {
    // First check if we know about this creature or not
    auto it = std::find(known_creatures->begin(), known_creatures->end(), creature->getCreatureId());
    if (it == known_creatures->end())
    {
      // Find an empty spot
      auto unused = std::find(known_creatures->begin(), known_creatures->end(), Creature::INVALID_ID);
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
  };

  const auto item_func = [&packet](const Item* item)
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
  };

  thing.visit(creature_func, item_func);
}

void addMapData(const WorldInterface& world_interface,
                const Position& position,
                int width,
                int height,
                KnownCreatures* known_creatures,
                OutgoingPacket* packet)
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
        const auto* tile = world_interface.getTile(Position(x, y, position.getZ()));
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

void addTileData(const Tile& tile, KnownCreatures* known_creatures, OutgoingPacket* packet)
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

void addOutfitData(const Outfit& outfit, OutgoingPacket* packet)
{
  packet->add(outfit.type);
  packet->add(outfit.head);
  packet->add(outfit.body);
  packet->add(outfit.legs);
  packet->add(outfit.feet);
}


protocol_types::Login getLogin(IncomingPacket* packet)
{
  protocol_types::Login login;
  packet->get(&login.unknown1);
  packet->get(&login.client_os);
  packet->get(&login.client_version);
  packet->get(&login.unknown2);
  packet->get(&login.character_name);
  packet->get(&login.password);
  return login;
}

protocol_types::MoveClick getMoveClick(IncomingPacket* packet)
{
  protocol_types::MoveClick move;
  const auto length = packet->getU8();
  for (auto i = 0U; i < length; i++)
  {
    move.path.push_back(static_cast<Direction>(packet->getU8()));
  }
  return move;
}

protocol_types::MoveItem getMoveItem(KnownContainers* container_ids, IncomingPacket* packet)
{
  protocol_types::MoveItem move;
  move.from_item_position = getItemPosition(container_ids, packet);
  move.to_game_position = getGamePosition(container_ids, packet);
  packet->get(&move.count);
  return move;
}

protocol_types::UseItem getUseItem(KnownContainers* container_ids, IncomingPacket* packet)
{
  protocol_types::UseItem use;
  use.item_position = getItemPosition(container_ids, packet);
  packet->get(&use.new_container_id);
  return use;
}

protocol_types::CloseContainer getCloseContainer(IncomingPacket* packet)
{
  protocol_types::CloseContainer close;
  packet->get(&close.container_id);
  return close;
}

protocol_types::OpenParentContainer getOpenParentContainer(IncomingPacket* packet)
{
  protocol_types::OpenParentContainer open;
  packet->get(&open.container_id);
  return open;
}

protocol_types::LookAt getLookAt(KnownContainers* container_ids, IncomingPacket* packet)
{
  protocol_types::LookAt look;
  look.item_position = getItemPosition(container_ids, packet);
  return look;
}

protocol_types::Say getSay(IncomingPacket* packet)
{
  protocol_types::Say say;
  packet->get(&say.type);
  switch (say.type)
  {
    case 0x06:  // PRIVATE
    case 0x0B:  // PRIVATE RED
      packet->get(&say.receiver);
      break;
    case 0x07:  // CHANNEL_Y
    case 0x0A:  // CHANNEL_R1
      packet->get(&say.channel_id);
      break;
    default:
      break;
  }
  packet->get(&say.message);
  return say;
}

GamePosition getGamePosition(KnownContainers* container_ids, IncomingPacket* packet)
{
  const auto x = packet->getU16();
  const auto y = packet->getU16();
  const auto z = packet->getU8();

  LOG_DEBUG("%s: x = 0x%04X, y = 0x%04X, z = 0x%02X", __func__, x, y, z);

  if (x != 0xFFFF)
  {
    // Positions have x not fully set
    return GamePosition(Position(x, y, z));
  }

  if ((y & 0x40) == 0x00)
  {
    // Inventory have x fully set and 7th bit in y not set
    // Inventory slot is 4 lower bits in y
    return GamePosition(y & ~0x40);
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
  if (item_unique_id == Item::INVALID_UNIQUE_ID)
  {
    LOG_ERROR("%s: container_id does not map to a valid ItemUniqueId: %d", __func__, container_id);
    return {};
  }

  return { item_unique_id, z };
}

ItemPosition getItemPosition(KnownContainers* container_ids, IncomingPacket* packet)
{
  const auto game_position = getGamePosition(container_ids, packet);
  const auto item_id = packet->getU16();
  const auto stackpos = packet->getU8();

  return { game_position, item_id, stackpos };
}

}  // namespace protocol_helper
