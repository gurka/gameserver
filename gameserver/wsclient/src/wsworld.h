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

#ifndef WSCLIENT_SRC_WSWORLD_H_
#define WSCLIENT_SRC_WSWORLD_H_

#include <array>
#include <vector>

#include "tiles.h"
#include "position.h"
#include "creature.h"
#include "item.h"
#include "protocol_common.h"
#include "protocol_client.h"
#include "data_loader.h"

#include "types.h"

namespace wsclient::wsworld
{

struct Creature
{
  common::CreatureId id;
  std::string name;
  std::uint8_t health_percent;
  common::Direction direction;
  common::Outfit outfit;
  std::uint16_t speed;
};

class Map
{
 public:
  void setItemTypes(const utils::data_loader::ItemTypes* itemtypes) { m_itemtypes = itemtypes; }
  void setPlayerId(common::CreatureId player_id) { m_player_id = player_id; }

  // Methods that work with protocol objects
  void setFullMapData(const protocol::client::FullMap& map_data);
  void setPartialMapData(const protocol::client::PartialMap& map_data);
  void updateTile(const protocol::client::TileUpdate& tile_update);
  void handleFloorChange(bool up, const protocol::client::FloorChange& floor_change);
  void addProtocolThing(const common::Position& position, const protocol::Thing& thing);

  // Methods that does not work with protocol objects
  void addThing(const common::Position& position, Thing thing);
  void removeThing(const common::Position& position, std::uint8_t stackpos);
  void updateThing(const common::Position& position,
                   std::uint8_t stackpos,
                   const protocol::Thing& thing);
  void moveThing(const common::Position& from_position,
                 std::uint8_t from_stackpos,
                 const common::Position& to_position);

  void setCreatureSkull(common::CreatureId creature_id, std::uint8_t skull);

  const common::Position& getPlayerPosition() const { return m_tiles.getMapPosition(); }
  const auto& getTiles() const { return m_tiles.getTiles(); }
  int getNumFloors() const { return m_tiles.getNumFloors(); }
  const Tile* getTile(const common::Position& position) const;
  const Creature* getCreature(common::CreatureId creature_id) const;
  bool ready() const { return m_ready; }

 private:
  // Methods that work protocol objects
  Thing parseThing(const protocol::Thing& thing);
  void setTile(const protocol::Tile& protocol_tile, Tile* world_tile);

  // Methods that does not work with protocol objects
  Creature* getCreature(common::CreatureId creature_id);
  Thing getThing(const common::Position& position, std::uint8_t stackpos);

  Tiles m_tiles;
  const utils::data_loader::ItemTypes* m_itemtypes;
  common::CreatureId m_player_id = 0U;
  bool m_ready = false;
  std::vector<Creature> m_known_creatures;
};

}  // namespace wsclient::wsworld

#endif  // WSCLIENT_SRC_WSWORLD_H_
