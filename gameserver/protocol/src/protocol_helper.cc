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

bool canSee(const Position& player_position, const Position& to_position)
{
  // Note: client displays 15x11 tiles, but it know about 18x14 tiles.
  //
  //       Client know about one extra row north, one extra column west
  //       two extra rows south and two extra rows east.
  //
  //       This function returns true if to_position is visible from player_position
  //       with regards to what the client (player_position) knows about, e.g. 18x14 tiles.
  //
  //
  //     00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18
  //     ________________________________________________________
  // 00 |   _______________________________________________      |
  // 01 |  |                                               |     |
  // 02 |  |                                               |     |
  // 03 |  |                                               |     |
  // 04 |  |                                               |     |
  // 05 |  |                                               |     |
  // 06 |  |                                               |     |
  // 07 |  |                                               |     |
  // 08 |  |                                               |     |
  // 09 |  |                                               |     |
  // 10 |  |                                               |     |
  // 11 |  |                                               |     |
  // 12 |  |_______________________________________________|     |
  // 13 |                                                        |
  // 14 |________________________________________________________|

  return to_position.getX() >= player_position.getX() - 8 &&
         to_position.getX() <= player_position.getX() + 9 &&
         to_position.getY() >= player_position.getY() - 6 &&
         to_position.getY() <= player_position.getY() + 7;
}

void addPosition(const Position& position, OutgoingPacket* packet)
{
  packet->add(position.getX());
  packet->add(position.getY());
  packet->add(position.getZ());
}

void addFullMapData(const WorldInterface& world_interface,
                    const Position& position,
                    std::array<CreatureId, 64>* knownCreatures,
                    OutgoingPacket* packet)
{
  packet->addU8(0x64);
  addPosition(position, packet);
  addMapData(world_interface,
             Position(position.getX() - 8, position.getY() - 6, position.getZ()),
             18,
             14,
             knownCreatures,
             packet);
}

void addMapData(const WorldInterface& world_interface,
                const Position& position,
                int width,
                int height,
                std::array<CreatureId, 64>* knownCreatures,
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

          const auto& items = tile->getItems();
          const auto& creatureIds = tile->getCreatureIds();
          auto itemIt = items.cbegin();
          auto creatureIt = creatureIds.cbegin();

          // Client can only handle ground + 9 items/creatures at most
          auto count = 0;

          // Add ground Item
          addItem(*(*itemIt), packet);
          count++;
          ++itemIt;

          // if splash; add; count++

          // Add top Items
          while (count < 10 && itemIt != items.cend())
          {
            if (!(*itemIt)->getItemType().alwaysOnTop)
            {
              break;
            }

            addItem(*(*itemIt), packet);
            count++;
            ++itemIt;
          }

          // Add Creatures
          while (count < 10 && creatureIt != creatureIds.cend())
          {
            const Creature& creature = world_interface.getCreature(*creatureIt);
            addCreature(creature, knownCreatures, packet);
            count++;
            ++creatureIt;
          }

          // Add bottom Item
          while (count < 10 && itemIt != items.cend())
          {
            addItem(*(*itemIt), packet);
            count++;
            ++itemIt;
          }
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

void addOutfit(const Outfit& outfit, OutgoingPacket* packet)
{
  packet->add(outfit.type);
  packet->add(outfit.head);
  packet->add(outfit.body);
  packet->add(outfit.legs);
  packet->add(outfit.feet);
}

void addCreature(const Creature& creature,
                 std::array<CreatureId, 64>* knownCreatures,
                 OutgoingPacket* packet)
{
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

    packet->addU8(0x61);
    packet->addU8(0x00);
    packet->addU32(0x00);  // creatureId to remove (0x00 = none)
    packet->add(creature.getCreatureId());
    packet->add(creature.getName());
  }
  else
  {
    // We already know about this creature
    packet->addU8(0x62);
    packet->addU8(0x00);
    packet->add(creature.getCreatureId());
  }

  packet->addU8(creature.getHealth() / creature.getMaxHealth() * 100);
  packet->add(static_cast<std::uint8_t>(creature.getDirection()));
  addOutfit(creature.getOutfit(), packet);

  packet->addU8(0x00);
  packet->addU8(0xDC);

  packet->add(creature.getSpeed());
}

void addItem(const Item& item, OutgoingPacket* packet)
{
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

void addEquipment(const Equipment& equipment, std::uint8_t inventoryIndex, OutgoingPacket* packet)
{
  const auto* item = equipment.getItem(inventoryIndex);
  if (!item)
  {
    packet->addU8(0x79);  // No Item in this slot
    packet->add(inventoryIndex);
  }
  else
  {
    packet->addU8(0x78);
    packet->add(inventoryIndex);
    addItem(*item, packet);
  }
}

void addMagicEffect(const Position& position,
                    std::uint8_t type,
                    OutgoingPacket* packet)
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
  packet->addU8(0xA1);
  // TODO(simon): add skills to Player
  (void)player;
  for (auto i = 0; i < 7; i++)
  {
    packet->addU8(10);
  }
}

void addWorldLight(std::uint8_t intensity,
                   std::uint8_t color,
                   OutgoingPacket* packet)
{
  packet->addU8(0x82);
  packet->add(intensity);
  packet->add(color);
}

Position getPosition(IncomingPacket* packet)
{
  const auto x = packet->getU16();
  const auto y = packet->getU16();
  const auto z = packet->getU8();
  return Position(x, y, z);
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

ProtocolTypes::Login getLogin(IncomingPacket* packet)
{
  ProtocolTypes::Login login;
  packet->get(&login.playerId);
  packet->get(&login.serverBeat);
  return login;
}

ProtocolTypes::LoginFailed getLoginFailed(IncomingPacket* packet)
{
  ProtocolTypes::LoginFailed failed;
  packet->get(&failed.reason);
  return failed;
}

ProtocolTypes::Creature getCreature(bool known, IncomingPacket* packet)
{
  ProtocolTypes::Creature creature;
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
  packet->get(&creature.speed);
  return creature;
}

ProtocolTypes::Item getItem(IncomingPacket* packet)
{
  ProtocolTypes::Item item;
  packet->get(&item.itemTypeId);
  // Need to make ItemType available to be able to check if we should
  // read extra or not. For now assume not to read it
  return item;
}

ProtocolTypes::Equipment getEquipment(bool empty, IncomingPacket* packet)
{
  ProtocolTypes::Equipment equipment;
  equipment.empty = empty;
  packet->get(&equipment.inventoryIndex);
  if (equipment.empty)
  {
    equipment.item = getItem(packet);
  }
  return equipment;
}

ProtocolTypes::MagicEffect getMagicEffect(IncomingPacket* packet)
{
  ProtocolTypes::MagicEffect effect;
  effect.position = getPosition(packet);
  packet->get(&effect.type);
  return effect;
}

ProtocolTypes::PlayerStats getPlayerStats(IncomingPacket* packet)
{
  ProtocolTypes::PlayerStats stats;
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

ProtocolTypes::WorldLight getWorldLight(IncomingPacket* packet)
{
  ProtocolTypes::WorldLight light;
  packet->get(&light.intensity);
  packet->get(&light.color);
  return light;
}

ProtocolTypes::PlayerSkills getPlayerSkills(IncomingPacket* packet)
{
  ProtocolTypes::PlayerSkills skills;
  packet->get(&skills.fist);
  packet->get(&skills.club);
  packet->get(&skills.sword);
  packet->get(&skills.axe);
  packet->get(&skills.dist);
  packet->get(&skills.shield);
  packet->get(&skills.fish);
  return skills;
}

ProtocolTypes::TextMessage getTextMessage(IncomingPacket* packet)
{
  ProtocolTypes::TextMessage message;
  packet->get(&message.type);
  packet->get(&message.message);
  return message;
}

ProtocolTypes::MapData getMapData(int width, int height, IncomingPacket* packet)
{
  ProtocolTypes::MapData map;

  map.position = ProtocolHelper::getPosition(packet);

  // Assume that we always are on z=7
  auto skip = 0;
  for (auto z = 7; z >= 0; z--)
  {
    for (auto x = 0; x < width; x++)
    {
      for (auto y = 0; y < height; y++)
      {
        ProtocolTypes::MapData::TileData tile;
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
            ProtocolTypes::MapData::CreatureData creature;
            creature.stackpos = stackpos;
            creature.creature = ProtocolHelper::getCreature(packet->getU16() == 0x0062, packet);
            tile.creatures.push_back(std::move(creature));
          }
          else
          {
            ProtocolTypes::MapData::ItemData item;
            item.stackpos = stackpos;
            item.item = ProtocolHelper::getItem(packet);
            tile.items.push_back(std::move(item));
          }
        }

        map.tiles.push_back(std::move(tile));
      }
    }
  }

  return map;
}

GamePosition getGamePosition(std::array<ItemUniqueId, 64>* containerIds, IncomingPacket* packet)
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

ItemPosition getItemPosition(std::array<ItemUniqueId, 64>* containerIds, IncomingPacket* packet)
{
  const auto gamePosition = getGamePosition(containerIds, packet);
  const auto itemId = packet->getU16();
  const auto stackPosition = packet->getU8();

  return ItemPosition(gamePosition, itemId, stackPosition);
}

}
