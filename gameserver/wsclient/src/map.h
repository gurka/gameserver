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
      bool isItem;
      union
      {
        CreatureId creatureId;
        struct
        {
          ItemTypeId itemTypeId;
          std::uint8_t extra;
          bool onTop;
        } item;
      };
    };

    std::vector<Thing> things;
  };

  void setMapData(const ProtocolTypes::MapData& mapData);
  void setPlayerPosition(const Position& position) { playerPosition_ = position; }

  void addCreature(const Position& position, CreatureId creatureId);
  void addItem(const Position& position, ItemTypeId itemTypeId, std::uint8_t extra, bool onTop);
  void removeThing(const Position& position, std::uint8_t stackpos);

  const Tile& getTile(const Position& position) const;

 private:
  Position playerPosition_;
  std::array<std::array<Tile, types::known_tiles_x>, types::known_tiles_y> tiles_;
};

#endif  // WSCLIENT_SRC_MAP_H_
