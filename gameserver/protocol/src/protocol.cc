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
#include "protocol.h"

#include <algorithm>

#include "logger.h"
#include "position.h"
#include "outgoing_packet.h"
#include "world.h"
#include "creature.h"
#include "item.h"
#include "player.h"
#include "incoming_packet.h"
#include "tile.h"

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

namespace server
{

void addLogin(common::CreatureId player_id, std::uint16_t server_beat, network::OutgoingPacket* packet)
{
  packet->addU8(0x0A);
  packet->add(player_id);
  packet->add(server_beat);
}

void addLoginFailed(const std::string& reason, network::OutgoingPacket* packet)
{
  packet->addU8(0x14);
  packet->add(reason);
}
void addMapFull(const world::World& world,
                const common::Position& position,
                KnownCreatures* known_creatures,
                network::OutgoingPacket* packet)
{
  packet->addU8(0x64);
  addPosition(position, packet);
  addMapData(world,
             common::Position(position.getX() - 8, position.getY() - 6, position.getZ()),
             18,
             14,
             known_creatures,
             packet);
}

void addMap(const world::World& world,
            const common::Position& old_position,
            const common::Position& new_position,
            KnownCreatures* known_creatures,
            network::OutgoingPacket* packet)
{
  if (old_position.getY() > new_position.getY())
  {
    // North
    packet->addU8(0x65);
    addMapData(world,
               common::Position(old_position.getX() - 8, new_position.getY() - 6, old_position.getZ()),
               18,
               1,
               known_creatures,
               packet);
  }
  else if (old_position.getY() < new_position.getY())
  {
    // South
    packet->addU8(0x67);
    addMapData(world,
               common::Position(old_position.getX() - 8, new_position.getY() + 7, old_position.getZ()),
               18,
               1,
               known_creatures,
               packet);
  }

  if (old_position.getX() > new_position.getX())
  {
    // West
    packet->addU8(0x68);
    addMapData(world,
               common::Position(new_position.getX() - 8, new_position.getY() - 6, old_position.getZ()),
               1,
               14,
               known_creatures,
               packet);
  }
  else if (old_position.getX() < new_position.getX())
  {
    // East
    packet->addU8(0x66);
    addMapData(world,
               common::Position(new_position.getX() + 9, new_position.getY() - 6, old_position.getZ()),
               1,
               14,
               known_creatures,
               packet);
  }
}

void addTileUpdated(const common::Position& position,
                    const world::World& world,
                    KnownCreatures* known_creatures,
                    network::OutgoingPacket* packet)
{
  packet->addU8(0x69);
  addPosition(position, packet);
  const auto* tile = world.getTile(position);
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

void addThingAdded(const common::Position& position,
                   const common::Thing& thing,
                   KnownCreatures* known_creatures,
                   network::OutgoingPacket* packet)
{
  packet->addU8(0x6A);
  addPosition(position, packet);
  addThing(thing, known_creatures, packet);
}

void addThingChanged(const common::Position& position,
                     std::uint8_t stackpos,
                     const common::Thing& thing,
                     KnownCreatures* known_creatures,
                     network::OutgoingPacket* packet)
{
  (void)known_creatures;

  packet->addU8(0x6B);
  addPosition(position, packet);
  packet->add(stackpos);

  // TODO(simon): fix this, also see addCreature
  if (thing.creature())
  {
    packet->addU16(0x0063);
    packet->add(thing.creature()->getCreatureId());
    packet->add(static_cast<std::uint8_t>(thing.creature()->getDirection()));
  }
  else
  {
    addItem(thing.item(), packet);
  }
}

void addThingRemoved(const common::Position& position, std::uint8_t stackpos, network::OutgoingPacket* packet)
{
  packet->addU8(0x6C);
  addPosition(position, packet);
  packet->add(stackpos);
}

void addThingMoved(const common::Position& old_position,
                   std::uint8_t old_stackpos,
                   const common::Position& new_position,
                   network::OutgoingPacket* packet)
{
  packet->addU8(0x6D);
  addPosition(old_position, packet);
  packet->add(old_stackpos);
  addPosition(new_position, packet);
}

void addContainerOpen(std::uint8_t container_id,
                      const common::Thing& thing,
                      const gameengine::Container& container,
                      network::OutgoingPacket* packet)
{
  packet->addU8(0x6E);
  packet->add(container_id);
  addThing(thing, nullptr, packet);
  packet->add(thing.item()->getItemType().name);
  packet->add(thing.item()->getItemType().maxitems);
  packet->addU8(container.parent_item_unique_id == common::Item::INVALID_UNIQUE_ID ? 0x00 : 0x01);
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

void addContainerClose(std::uint8_t container_id, network::OutgoingPacket* packet)
{
  packet->addU8(0x6F);
  packet->add(container_id);
}

void addContainerAddItem(std::uint8_t container_id, const common::Thing& thing, network::OutgoingPacket* packet)
{
  packet->addU8(0x70);
  packet->add(container_id);
  addThing(thing, nullptr, packet);
}

void addContainerUpdateItem(std::uint8_t container_id,
                            std::uint8_t container_slot,
                            const common::Thing& thing,
                            network::OutgoingPacket* packet)
{
  packet->addU8(0x71);
  packet->add(container_id);
  packet->add(container_slot);
  addThing(thing, nullptr, packet);
}

void addContainerRemoveItem(std::uint8_t container_id,
                            std::uint8_t container_slot,
                            network::OutgoingPacket* packet)
{
  packet->addU8(0x72);
  packet->add(container_id);
  packet->add(container_slot);
}

void addEquipmentUpdated(const gameengine::Equipment& equipment, std::uint8_t inventory_index, network::OutgoingPacket* packet)
{
  const auto* item = equipment.getItem(inventory_index);
  if (item)
  {
    packet->addU8(0x78);
    packet->add(inventory_index);
    addThing(common::Thing(item), nullptr, packet);
  }
  else
  {
    packet->addU8(0x79);  // No Item in this slot
    packet->add(inventory_index);
  }
}

void addWorldLight(std::uint8_t intensity, std::uint8_t color, network::OutgoingPacket* packet)
{
  packet->addU8(0x82);
  packet->add(intensity);
  packet->add(color);
}

void addMagicEffect(const common::Position& position, std::uint8_t type, network::OutgoingPacket* packet)
{
  packet->addU8(0x83);
  addPosition(position, packet);
  packet->add(type);
}

void addPlayerStats(const gameengine::Player& player, network::OutgoingPacket* packet)
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

void addPlayerSkills(const gameengine::Player& player, network::OutgoingPacket* packet)
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
             const common::Position& position,
             const std::string& message,
             network::OutgoingPacket* packet)
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

void addTextMessage(std::uint8_t type, const std::string& text, network::OutgoingPacket* packet)
{
  packet->addU8(0xB4);
  packet->add(type);
  packet->add(text);
}

void addCancelMove(network::OutgoingPacket* packet)
{
  packet->addU8(0xB5);
}

void addPosition(const common::Position& position, network::OutgoingPacket* packet)
{
  packet->add(position.getX());
  packet->add(position.getY());
  packet->add(position.getZ());
}

void addThing(const common::Thing& thing, KnownCreatures* known_creatures, network::OutgoingPacket* packet)
{
  thing.visit(
    [&known_creatures, &packet](const common::Creature* creature)
    {
      addCreature(creature, known_creatures, packet);
    },
    [&packet](const common::Item* item)
    {
      addItem(item, packet);
    }
  );
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

Login getLogin(network::IncomingPacket* packet)
{
  Login login;
  packet->get(&login.unknown1);
  packet->get(&login.client_os);
  packet->get(&login.client_version);
  packet->get(&login.unknown2);
  packet->get(&login.character_name);
  packet->get(&login.password);
  return login;
}

MoveClick getMoveClick(network::IncomingPacket* packet)
{
  MoveClick move;
  const auto length = packet->getU8();
  for (auto i = 0U; i < length; i++)
  {
    move.path.push_back(static_cast<common::Direction>(packet->getU8()));
  }
  return move;
}

MoveItem getMoveItem(KnownContainers* container_ids, network::IncomingPacket* packet)
{
  MoveItem move;
  move.from_item_position = getItemPosition(container_ids, packet);
  move.to_game_position = getGamePosition(container_ids, packet);
  packet->get(&move.count);
  return move;
}

UseItem getUseItem(KnownContainers* container_ids, network::IncomingPacket* packet)
{
  UseItem use;
  use.item_position = getItemPosition(container_ids, packet);
  packet->get(&use.new_container_id);
  return use;
}

CloseContainer getCloseContainer(network::IncomingPacket* packet)
{
  CloseContainer close;
  packet->get(&close.container_id);
  return close;
}

OpenParentContainer getOpenParentContainer(network::IncomingPacket* packet)
{
  OpenParentContainer open;
  packet->get(&open.container_id);
  return open;
}

LookAt getLookAt(KnownContainers* container_ids, network::IncomingPacket* packet)
{
  LookAt look;
  look.item_position = getItemPosition(container_ids, packet);
  return look;
}

Say getSay(network::IncomingPacket* packet)
{
  Say say;
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

}  // namespace server

namespace client
{

Login getLogin(network::IncomingPacket* packet)
{
  Login login;
  packet->get(&login.player_id);
  packet->get(&login.server_beat);
  return login;
}

LoginFailed getLoginFailed(network::IncomingPacket* packet)
{
  LoginFailed failed;
  packet->get(&failed.reason);
  return failed;
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

Equipment getEquipment(bool empty, network::IncomingPacket* packet)
{
  Equipment equipment;
  equipment.empty = empty;
  packet->get(&equipment.inventory_index);
  if (equipment.empty)
  {
    equipment.item = getItem(packet);
  }
  return equipment;
}

MagicEffect getMagicEffect(network::IncomingPacket* packet)
{
  MagicEffect effect;
  effect.position = getPosition(packet);
  packet->get(&effect.type);
  return effect;
}

PlayerStats getPlayerStats(network::IncomingPacket* packet)
{
  PlayerStats stats;
  packet->get(&stats.health);
  packet->get(&stats.max_health);
  packet->get(&stats.capacity);
  packet->get(&stats.exp);
  packet->get(&stats.level);
  packet->get(&stats.mana);
  packet->get(&stats.max_mana);
  packet->get(&stats.magic_level);
  return stats;
}

WorldLight getWorldLight(network::IncomingPacket* packet)
{
  WorldLight light;
  packet->get(&light.intensity);
  packet->get(&light.color);
  return light;
}

PlayerSkills getPlayerSkills(network::IncomingPacket* packet)
{
  PlayerSkills skills;
  packet->get(&skills.fist);
  packet->get(&skills.club);
  packet->get(&skills.sword);
  packet->get(&skills.axe);
  packet->get(&skills.dist);
  packet->get(&skills.shield);
  packet->get(&skills.fish);
  return skills;
}

TextMessage getTextMessage(network::IncomingPacket* packet)
{
  TextMessage message;
  packet->get(&message.type);
  packet->get(&message.message);
  return message;
}

MapData getMapData(int width, int height, network::IncomingPacket* packet)
{
  MapData map;

  map.position = getPosition(packet);

  // Assume that we always are on z=7
  auto skip = 0;
  for (auto z = 7; z >= 0; z--)
  {
    for (auto x = 0; x < width; x++)
    {
      for (auto y = 0; y < height; y++)
      {
        MapData::TileData tile;
        if (skip > 0)
        {
          skip -= 1;
          tile.skip = true;
          map.tiles.push_back(std::move(tile));
          continue;
        }

        // Parse tile
        tile.skip = false;
        for (auto stackpos = 0; true; stackpos++)
        {
          if (packet->peekU16() >= 0xFF00)
          {
            skip = packet->getU16() & 0xFF;
            break;
          }

          if (stackpos > 10)
          {
            LOG_ERROR("%s: too many things on this tile", __func__);
          }

          if (packet->peekU16() == 0x0061 ||
              packet->peekU16() == 0x0062)
          {
            MapData::CreatureData creature;
            creature.stackpos = stackpos;
            creature.creature = getCreature(packet->getU16() == 0x0062, packet);
            tile.creatures.push_back(std::move(creature));
          }
          else
          {
            MapData::ItemData item;
            item.stackpos = stackpos;
            item.item = getItem(packet);
            tile.items.push_back(item);
          }
        }

        map.tiles.push_back(std::move(tile));
      }
    }
  }

  return map;
}

CreatureMove getCreatureMove(bool can_see_old_pos, bool can_see_new_pos, network::IncomingPacket* packet)
{
  CreatureMove move;
  move.can_see_old_pos = can_see_old_pos;
  move.can_see_new_pos = can_see_new_pos;
  if (move.can_see_old_pos && move.can_see_new_pos)
  {
    move.old_position = getPosition(packet);
    packet->get(&move.old_stackpos);
    move.new_position = getPosition(packet);
  }
  else if (move.can_see_old_pos)
  {
    move.old_position = getPosition(packet);
    packet->get(&move.old_stackpos);
  }
  else  // if (move.can_see_new_pos)
  {
    move.new_position = getPosition(packet);
    move.creature = getCreature(packet->getU16() == 0x0062, packet);
  }
  return move;
}

}  // namespace client

}  // namespace protocol
