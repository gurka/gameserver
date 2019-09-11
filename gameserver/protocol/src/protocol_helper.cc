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

namespace ProtocolHelper
{

void addLogin(CreatureId playerId, std::uint16_t serverBeat, OutgoingPacket* packet)
{
  packet->addU8(0x0A);
  packet->add(playerId);
  packet->add(serverBeat);
}

void addLoginFailed(const std::string& reason, OutgoingPacket* packet)
{
  packet->addU8(0x14);
  packet->add(reason);
}
void addMapFull(const WorldInterface& worldInterface,
                const Position& position,
                KnownCreatures* knownCreatures,
                OutgoingPacket* packet)
{
  packet->addU8(0x64);
  addPosition(position, packet);
  addMapData(worldInterface,
             Position(position.getX() - 8, position.getY() - 6, position.getZ()),
             18,
             14,
             knownCreatures,
             packet);
}

void addMap(const WorldInterface& worldInterface,
            const Position& oldPosition,
            const Position& newPosition,
            KnownCreatures* knownCreatures,
            OutgoingPacket* packet)
{
  if (oldPosition.getY() > newPosition.getY())
  {
    // North
    packet->addU8(0x65);
    addMapData(worldInterface,
               Position(oldPosition.getX() - 8, newPosition.getY() - 6, oldPosition.getZ()),
               18,
               1,
               knownCreatures,
               packet);
  }
  else if (oldPosition.getY() < newPosition.getY())
  {
    // South
    packet->addU8(0x67);
    addMapData(worldInterface,
               Position(oldPosition.getX() - 8, newPosition.getY() + 7, oldPosition.getZ()),
               18,
               1,
               knownCreatures,
               packet);
  }

  if (oldPosition.getX() > newPosition.getX())
  {
    // West
    packet->addU8(0x68);
    addMapData(worldInterface,
               Position(newPosition.getX() - 8, newPosition.getY() - 6, oldPosition.getZ()),
               1,
               14,
               knownCreatures,
               packet);
  }
  else if (oldPosition.getX() < newPosition.getX())
  {
    // East
    packet->addU8(0x66);
    addMapData(worldInterface,
               Position(newPosition.getX() + 9, newPosition.getY() - 6, oldPosition.getZ()),
               1,
               14,
               knownCreatures,
               packet);
  }
}

void addTileUpdated(const Position& position,
                    const WorldInterface& worldInterface,
                    KnownCreatures* knownCreatures,
                    OutgoingPacket* packet)
{
  packet->addU8(0x69);
  addPosition(position, packet);
  const auto* tile = worldInterface.getTile(position);
  if (tile)
  {
    addTileData(*tile, knownCreatures, packet);
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
                   KnownCreatures* knownCreatures,
                   OutgoingPacket* packet)
{
  packet->addU8(0x6A);
  addPosition(position, packet);
  addThing(thing, knownCreatures, packet);
}

void addThingChanged(const Position& position,
                     std::uint8_t stackpos,
                     const Thing& thing,
                     KnownCreatures* knownCreatures,
                     OutgoingPacket* packet)
{
  packet->addU8(0x6B);
  addPosition(position, packet);
  packet->add(stackpos);
  addThing(thing, knownCreatures, packet);

  // Only for creature 0x0063?
  if (thing.creature)
  {
    packet->add(static_cast<std::uint8_t>(thing.creature->getDirection()));
  }
}

void addThingRemoved(const Position& position, std::uint8_t stackpos, OutgoingPacket* packet)
{
  packet->addU8(0x6C);
  addPosition(position, packet);
  packet->add(stackpos);
}

void addThingMoved(const Position& oldPosition,
                   std::uint8_t oldStackpos,
                   const Position& newPosition,
                   OutgoingPacket* packet)
{
  packet->addU8(0x6D);
  addPosition(oldPosition, packet);
  packet->add(oldStackpos);
  addPosition(newPosition, packet);
}

void addContainerOpen(std::uint8_t containerId,
                      const Thing& thing,
                      const Container& container,
                      OutgoingPacket* packet)
{
  packet->addU8(0x6E);
  packet->add(containerId);
  addThing(thing, nullptr, packet);
  packet->add(thing.item->getItemType().name);
  packet->add(thing.item->getItemType().maxitems);
  packet->addU8(container.parentItemUniqueId == Item::INVALID_UNIQUE_ID ? 0x00 : 0x01);
  packet->addU8(container.items.size());
  for (const auto* item : container.items)
  {
    packet->add(item->getItemTypeId());
    if (item->getItemType().isStackable)  // or splash or fluid container?
    {
      packet->add(item->getCount());
    }
  }
}

void addContainerClose(std::uint8_t containerId, OutgoingPacket* packet)
{
  packet->addU8(0x6F);
  packet->add(containerId);
}

void addContainerAddItem(std::uint8_t containerId, const Thing& thing, OutgoingPacket* packet)
{
  packet->addU8(0x70);
  packet->add(containerId);
  addThing(thing, nullptr, packet);
}

void addContainerUpdateItem(std::uint8_t containerId,
                            std::uint8_t containerSlot,
                            const Thing& thing,
                            OutgoingPacket* packet)
{
  packet->addU8(0x71);
  packet->add(containerId);
  packet->add(containerSlot);
  addThing(thing, nullptr, packet);
}

void addContainerRemoveItem(std::uint8_t containerId,
                            std::uint8_t containerSlot,
                            OutgoingPacket* packet)
{
  packet->addU8(0x72);
  packet->add(containerId);
  packet->add(containerSlot);
}

void addEquipmentUpdated(const Equipment& equipment, std::uint8_t inventoryIndex, OutgoingPacket* packet)
{
  const auto* item = equipment.getItem(inventoryIndex);
  if (item)
  {
    packet->addU8(0x78);
    packet->add(inventoryIndex);
    addThing(Thing(item), nullptr, packet);
  }
  else
  {
    packet->addU8(0x79);  // No Item in this slot
    packet->add(inventoryIndex);
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
              KnownCreatures* knownCreatures,
              OutgoingPacket* packet)
{
  if (thing.item)
  {
    const auto& item = *thing.item;

    packet->add(item.getItemTypeId());
    if (item.getItemType().isStackable)
    {
      packet->add(item.getCount());
    }
    else if (item.getItemType().isMultitype)
    {
      // TODO(simon): getSubType???
      packet->addU8(0);
    }
  }
  else  // thing.creature
  {
    const auto& creature = *thing.creature;

    // First check if we know about this creature or not
    auto it = std::find(knownCreatures->begin(), knownCreatures->end(), creature.getCreatureId());
    if (it == knownCreatures->end())
    {
      // Find an empty spot
      auto unused = std::find(knownCreatures->begin(), knownCreatures->end(), Creature::INVALID_ID);
      if (unused == knownCreatures->end())
      {
        // No empty spot!
        // TODO(simon): Figure out how to handle this - related to "creatureId to remove" below?
        LOG_ERROR("%s: knownCreatures_ is full!", __func__);
        return;
      }
      else
      {
        *unused = creature.getCreatureId();
      }

      packet->addU16(0x0061);  // UnknownCreature
      packet->addU32(0x00);  // creatureId to remove (0x00 = none)
      packet->add(creature.getCreatureId());
      packet->add(creature.getName());
    }
    else
    {
      // Client already know about this creature
      packet->addU16(0x0062);  // OutdatedCreature
      packet->add(creature.getCreatureId());
    }
    // TODO(simon): handle 0x0063 // Creature (+ direction)

    // TODO(simon): This is only for 0x0061 and 0x0062...
    packet->addU8(creature.getHealth() / creature.getMaxHealth() * 100);
    packet->add(static_cast<std::uint8_t>(creature.getDirection()));
    addOutfitData(creature.getOutfit(), packet);

    packet->addU8(0x00);
    packet->addU8(0xDC);

    packet->add(creature.getSpeed());
  }
}

void addMapData(const WorldInterface& worldInterface,
                const Position& position,
                int width,
                int height,
                KnownCreatures* knownCreatures,
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
        const auto* tile = worldInterface.getTile(Position(x, y, position.getZ()));
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

          addTileData(*tile, knownCreatures, packet);
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

void addTileData(const Tile& tile, KnownCreatures* knownCreatures, OutgoingPacket* packet)
{
  auto count = 0;
  for (const auto& thing : tile.getThings())
  {
    if (count >= 10)
    {
      break;
    }

    addThing(thing, knownCreatures, packet);
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


ProtocolTypes::Login getLogin(IncomingPacket* packet)
{
  ProtocolTypes::Login login;
  packet->get(&login.unknown1);
  packet->get(&login.clientOs);
  packet->get(&login.clientVersion);
  packet->get(&login.unknown2);
  packet->get(&login.characterName);
  packet->get(&login.password);
  return login;
}

ProtocolTypes::MoveClick getMoveClick(IncomingPacket* packet)
{
  ProtocolTypes::MoveClick move;
  const auto length = packet->getU8();
  for (auto i = 0u; i < length; i++)
  {
    move.path.push_back(static_cast<Direction>(packet->getU8()));
  }
  return move;
}

ProtocolTypes::MoveItem getMoveItem(KnownContainers* containerIds, IncomingPacket* packet)
{
  ProtocolTypes::MoveItem move;
  move.fromItemPosition = getItemPosition(containerIds, packet);
  move.toGamePosition = getGamePosition(containerIds, packet);
  packet->get(&move.count);
  return move;
}

ProtocolTypes::UseItem getUseItem(KnownContainers* containerIds, IncomingPacket* packet)
{
  ProtocolTypes::UseItem use;
  use.itemPosition = getItemPosition(containerIds, packet);
  packet->get(&use.newContainerId);
  return use;
}

ProtocolTypes::CloseContainer getCloseContainer(IncomingPacket* packet)
{
  ProtocolTypes::CloseContainer close;
  packet->get(&close.containerId);
  return close;
}

ProtocolTypes::OpenParentContainer getOpenParentContainer(IncomingPacket* packet)
{
  ProtocolTypes::OpenParentContainer open;
  packet->get(&open.containerId);
  return open;
}

ProtocolTypes::LookAt getLookAt(KnownContainers* containerIds, IncomingPacket* packet)
{
  ProtocolTypes::LookAt look;
  look.itemPosition = getItemPosition(containerIds, packet);
  return look;
}

ProtocolTypes::Say getSay(IncomingPacket* packet)
{
  ProtocolTypes::Say say;
  packet->get(&say.type);
  switch (say.type)
  {
    case 0x06:  // PRIVATE
    case 0x0B:  // PRIVATE RED
      packet->get(&say.receiver);
      break;
    case 0x07:  // CHANNEL_Y
    case 0x0A:  // CHANNEL_R1
      packet->get(&say.channelId);
      break;
    default:
      break;
  }
  packet->get(&say.message);
  return say;
}

ProtocolTypes::UseItem getUseItem(KnownContainers* containerIds, IncomingPacket* packet)
{
  ProtocolTypes::UseItem use;
  use.itemPosition = getItemPosition(containerIds, packet);
  packet->get(&use.newContainerId);
  return use;
}

ProtocolTypes::CloseContainer getCloseContainer(IncomingPacket* packet)
{
  ProtocolTypes::CloseContainer close;
  packet->get(&close.containerId);
  return close;
}

GamePosition getGamePosition(KnownContainers* containerIds, IncomingPacket* packet)
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
  else if ((y & 0x40) == 0x00)
  {
    // Inventory have x fully set and 7th bit in y not set
    // Inventory slot is 4 lower bits in y
    return GamePosition(y & ~0x40);
  }
  else
  {
    // Container have x fully set and 7th bit in y set
    // Container id is lower 6 bits in y
    // Container slot is z
    const auto containerId = y & ~0x40;
    if (containerId < 0 || containerId > 64)
    {
      LOG_ERROR("%s: invalid containerId: %d", __func__, containerId);
      return GamePosition();
    }
    const auto itemUniqueId = containerIds->at(containerId);
    if (itemUniqueId == Item::INVALID_UNIQUE_ID)
    {
      LOG_ERROR("%s: containerId does not map to a valid ItemUniqueId: %d", __func__, containerId);
      return GamePosition();
    }
    return GamePosition(itemUniqueId, z);
  }
}

ItemPosition getItemPosition(KnownContainers* containerIds, IncomingPacket* packet)
{
  const auto gamePosition = getGamePosition(containerIds, packet);
  const auto itemId = packet->getU16();
  const auto stackPosition = packet->getU8();

  return ItemPosition(gamePosition, itemId, stackPosition);
}

namespace Client
{

ProtocolTypes::Client::Login getLogin(IncomingPacket* packet)
{
  ProtocolTypes::Client::Login login;
  packet->get(&login.playerId);
  packet->get(&login.serverBeat);
  return login;
}

ProtocolTypes::Client::LoginFailed getLoginFailed(IncomingPacket* packet)
{
  ProtocolTypes::Client::LoginFailed failed;
  packet->get(&failed.reason);
  return failed;
}

ProtocolTypes::Client::Creature getCreature(bool known, IncomingPacket* packet)
{
  ProtocolTypes::Client::Creature creature;
  creature.known = known;
  if (creature.known)
  {
    packet->get(&creature.id);
  }
  else
  {
    packet->get(&creature.idToRemove);
    packet->get(&creature.id);
    packet->get(&creature.name);
  }
  packet->get(&creature.healthPercent);
  creature.direction = static_cast<Direction>(packet->getU8());
  creature.outfit = getOutfit(packet);
  packet->getU16();  // unknown 0xDC00
  packet->get(&creature.speed);
  return creature;
}

ProtocolTypes::Client::Item getItem(IncomingPacket* packet)
{
  ProtocolTypes::Client::Item item;
  packet->get(&item.itemTypeId);
  // Need to make ItemType available to be able to check if we should
  // read extra or not. For now assume not to read it
  return item;
}

ProtocolTypes::Client::Equipment getEquipment(bool empty, IncomingPacket* packet)
{
  ProtocolTypes::Client::Equipment equipment;
  equipment.empty = empty;
  packet->get(&equipment.inventoryIndex);
  if (equipment.empty)
  {
    case 0x06:  // PRIVATE
    case 0x0B:  // PRIVATE RED
      packet->get(&say.receiver);
      break;
    case 0x07:  // CHANNEL_Y
    case 0x0A:  // CHANNEL_R1
      packet->get(&say.channelId);
      break;
    default:
      break;
  }
  return equipment;
}

ProtocolTypes::Client::MagicEffect getMagicEffect(IncomingPacket* packet)
{
  ProtocolTypes::Client::MagicEffect effect;
  effect.position = getPosition(packet);
  packet->get(&effect.type);
  return effect;
}

ProtocolTypes::Client::PlayerStats getPlayerStats(IncomingPacket* packet)
{
  ProtocolTypes::Client::PlayerStats stats;
  packet->get(&stats.health);
  packet->get(&stats.maxHealth);
  packet->get(&stats.capacity);
  packet->get(&stats.exp);
  packet->get(&stats.level);
  packet->get(&stats.mana);
  packet->get(&stats.maxMana);
  packet->get(&stats.magicLevel);
  return stats;
}

ProtocolTypes::Client::WorldLight getWorldLight(IncomingPacket* packet)
{
  ProtocolTypes::Client::WorldLight light;
  packet->get(&light.intensity);
  packet->get(&light.color);
  return light;
}

ProtocolTypes::Client::PlayerSkills getPlayerSkills(IncomingPacket* packet)
{
  ProtocolTypes::Client::PlayerSkills skills;
  packet->get(&skills.fist);
  packet->get(&skills.club);
  packet->get(&skills.sword);
  packet->get(&skills.axe);
  packet->get(&skills.dist);
  packet->get(&skills.shield);
  packet->get(&skills.fish);
  return skills;
}

ProtocolTypes::Client::TextMessage getTextMessage(IncomingPacket* packet)
{
  ProtocolTypes::Client::TextMessage message;
  packet->get(&message.type);
  packet->get(&message.message);
  return message;
}

ProtocolTypes::Client::MapData getMapData(int width, int height, IncomingPacket* packet)
{
  ProtocolTypes::Client::MapData map;

  map.position = getPosition(packet);

  // Assume that we always are on z=7
  auto skip = 0;
  for (auto z = 7; z >= 0; z--)
  {
    for (auto x = 0; x < width; x++)
    {
      for (auto y = 0; y < height; y++)
      {
        ProtocolTypes::Client::MapData::TileData tile;
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
            ProtocolTypes::Client::MapData::CreatureData creature;
            creature.stackpos = stackpos;
            creature.creature = getCreature(packet->getU16() == 0x0062, packet);
            tile.creatures.push_back(std::move(creature));
          }
          else
          {
            ProtocolTypes::Client::MapData::ItemData item;
            item.stackpos = stackpos;
            item.item = getItem(packet);
            tile.items.push_back(std::move(item));
          }
        }

        map.tiles.push_back(std::move(tile));
      }
    }
  }

  return map;
}

ProtocolTypes::Client::CreatureMove getCreatureMove(bool canSeeOldPos, bool canSeeNewPos, IncomingPacket* packet)
{
  ProtocolTypes::Client::CreatureMove move;
  move.canSeeOldPos = canSeeOldPos;
  move.canSeeNewPos = canSeeNewPos;
  if (move.canSeeOldPos && move.canSeeNewPos)
  {
    move.oldPosition = getPosition(packet);
    packet->get(&move.oldStackPosition);
    move.newPosition = getPosition(packet);
  }
  else if (move.canSeeOldPos)
  {
    move.oldPosition = getPosition(packet);
    packet->get(&move.oldStackPosition);
  }
  else  // if (move.canSeeNewPos)
  {
    move.newPosition = getPosition(packet);
    move.creature = getCreature(packet->getU16() == 0x0062, packet);
  }
  return move;
}

Outfit getOutfit(IncomingPacket* packet)
{
  Outfit outfit;
  packet->get(&outfit.type);
  packet->get(&outfit.head);
  packet->get(&outfit.body);
  packet->get(&outfit.legs);
  packet->get(&outfit.feet);
  return outfit;
}

}

}  // namespace ProtocolHelper
