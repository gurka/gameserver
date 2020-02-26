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

#include "position.h"
#include "creature.h"
#include "item.h"
#include "protocol_common.h"
#include "protocol_client.h"
#include "data_loader.h"

#include "types.h"

namespace wsclient::wsworld
{

class Map
{
 public:
  struct Creature
  {
    common::CreatureId id;
    std::string name;
    std::uint8_t health_percent;
    common::Direction direction;
    common::Outfit outfit;
    std::uint16_t speed;
  };

  struct Item
  {
    const common::ItemType* type;
    std::uint8_t extra;
  };

  using Thing = std::variant<common::CreatureId, Item>;

  struct Tile
  {
    std::vector<Thing> things;
  };

  void setItemTypes(const io::data_loader::ItemTypes* itemtypes) { m_itemtypes = itemtypes; }
  void setMapData(const protocol::client::Map& map_data);
  void setPlayerPosition(const common::Position& position) { m_player_position = position; }

  void addCreature(const common::Position& position, common::CreatureId creature_id);
  void addCreature(const common::Position& position, const protocol::Creature& creature);
  void addItem(const common::Position& position,
               common::ItemTypeId item_type_id,
               std::uint8_t extra,
               bool onTop);
  void removeThing(const common::Position& position, std::uint8_t stackpos);

  const Tile& getTile(const common::Position& position) const;

  bool ready() const { return m_ready; }

  const Creature* getCreature(common::CreatureId creature_id) const;

 private:
  Creature* getCreature(common::CreatureId creature_id);

  const io::data_loader::ItemTypes* m_itemtypes;
  common::Position m_player_position = { 0, 0, 0 };
  std::array<std::array<Tile, consts::known_tiles_x>, consts::known_tiles_y> m_tiles;
  bool m_ready = false;
  std::vector<Creature> m_known_creatures;
};

}  // namespace wsclient::wsworld

#endif  // WSCLIENT_SRC_WSWORLD_H_
