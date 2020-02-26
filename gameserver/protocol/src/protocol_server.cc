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
#include "protocol_server.h"

#include <cstdint>

#include "logger.h"
#include "creature.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "world.h"
#include "position.h"
#include "container.h"
#include "player.h"

namespace protocol::server
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

}  // namespace protocol::server
