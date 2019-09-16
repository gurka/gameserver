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
        world::CreatureId creature_id;
        struct
        {
          world::ItemTypeId item_type_id;
          std::uint8_t extra;
          bool onTop;
        } item;
      };
    };

    std::vector<Thing> things;
  };

  void setMapData(const protocol::client::MapData& mapData);
  void setPlayerPosition(const world::Position& position) { m_player_position = position; }

  void addCreature(const world::Position& position, world::CreatureId creatureId);
  void addItem(const world::Position& position, world::ItemTypeId itemTypeId, std::uint8_t extra, bool onTop);
  void removeThing(const world::Position& position, std::uint8_t stackpos);

  const Tile& getTile(const world::Position& position) const;

 private:
  world::Position m_player_position = { 0, 0, 0 };
  std::array<std::array<Tile, types::known_tiles_x>, types::known_tiles_y> m_tiles;
};

#endif  // WSCLIENT_SRC_MAP_H_
