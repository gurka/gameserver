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
#ifndef WSCLIENT_SRC_MAP_H_
#define WSCLIENT_SRC_MAP_H_

#include <array>
#include <vector>

#include "position.h"
#include "creature.h"
#include "item.h"
#include "protocol_types.h"

#include "types.h"

namespace wsclient::world
{

using ::world::Position;
using ::world::CreatureId;
using ::world::ItemTypeId;

class Map
{
 public:
  struct Tile
  {
    struct Thing
    {
      bool is_item;
      union
      {
        CreatureId creature_id;
        struct
        {
          ItemTypeId item_type_id;
          std::uint8_t extra;
          bool onTop;
        } item;
      };
    };

    std::vector<Thing> things;
  };

  void setMapData(const protocol::client::MapData& map_data);
  void setPlayerPosition(const Position& position) { m_player_position = position; }

  void addCreature(const Position& position, CreatureId creature_id);
  void addItem(const Position& position, ItemTypeId item_type_id, std::uint8_t extra, bool onTop);
  void removeThing(const Position& position, std::uint8_t stackpos);

  const Tile& getTile(const Position& position) const;

 private:
  Position m_player_position = { 0, 0, 0 };
  std::array<std::array<Tile, consts::known_tiles_x>, consts::known_tiles_y> m_tiles;
};

}  // namespace wsclient::world

#endif  // WSCLIENT_SRC_MAP_H_