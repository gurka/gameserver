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

#ifndef WSCLIENT_SRC_TILES_H_
#define WSCLIENT_SRC_TILES_H_

#include <array>
#include <variant>

#include "position.h"
#include "item.h"
#include "creature.h"
#include "types.h"

namespace wsclient::wsworld
{

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

using TileArray = std::array<Tile, consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y * 8>;

class Tiles
{
 public:
  const common::Position& getMapPosition() const { return m_position; }
  void setMapPosition(const common::Position& position) { m_position = position; }

  void shiftTiles(common::Direction direction);

  Tile* getTileLocalPos(int local_x, int local_y, int local_z);
  const Tile* getTileLocalPos(int local_x, int local_y, int local_z) const;

  Tile* getTile(const common::Position& position);
  const Tile* getTile(const common::Position& position) const;

  const auto& getTiles() const { return m_tiles; }
  int getNumFloors() const
  {
    return  (m_position.getZ() <= 7) ? 8 :
           ((m_position.getZ() <= 13) ? 5 :
           ((m_position.getZ() <= 14) ? 4 : 3));
  }

 private:
  // This is the position of the middle Tile (-1, -1 as we know one extra column/row to the right/bottom)
  // 18x14 tiles, so the middle Tile has index 8, 6.
  common::Position m_position = { 0, 0, 0 };
  TileArray m_tiles;
};

}  // namespace wsclient::wsworld

#endif  // WSCLIENT_SRC_TILES_H_
