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
#include "logger.h"

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
  void swapFloors(int z1, int z2);
  void shiftFloorForwards(int num_floors);
  void shiftFloorBackwards(int num_floors);

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

  bool positionIsKnown(const common::Position& position) const
  {
    const auto z_min = (m_position.getZ() <= 7) ? 0 : (m_position.getZ() - 2);
    const auto z_max = (m_position.getZ() <= 7) ? 7 : (std::min(m_position.getZ() + 2, 15));

    const auto xy_offset = position.getZ() - m_position.getZ();

    const auto x_min = m_position.getX() - 8 - xy_offset;
    const auto x_max = m_position.getX() + 9 - xy_offset;

    const auto y_min = m_position.getY() - 6 - xy_offset;
    const auto y_max = m_position.getY() + 7 - xy_offset;

    return position.getX() >= x_min && position.getX() <= x_max &&
           position.getY() >= y_min && position.getY() <= y_max &&
           position.getZ() >= z_min && position.getZ() <= z_max;
  }

  common::Position globalToLocalPosition(const common::Position& position) const
  {
    if (!positionIsKnown(position))
    {
      LOG_ERROR("%s: global position is not known", __func__);
      return common::Position(-1, -1, -1);
    }

    const auto xy_offset = m_position.getZ() - position.getZ();
    const auto local_x = position.getX() + 8 - m_position.getX() - xy_offset;
    const auto local_y = position.getY() + 6 - m_position.getY() - xy_offset;
    const auto local_z = (m_position.getZ() <= 7) ? (7 - position.getZ()) : (position.getZ() - m_position.getZ() + 2);
    return common::Position(local_x, local_y, local_z);
  }

  common::Position localToGlobalPosition(const common::Position& position) const
  {
    const auto global_z = (m_position.getZ() <= 7) ? (7 - position.getZ()) : (position.getZ() + m_position.getZ() - 2);
    const auto xy_offset = m_position.getZ() - global_z;
    const auto global_x = position.getX() - 8 + m_position.getX() + xy_offset;
    const auto global_y = position.getY() - 6 + m_position.getY() + xy_offset;
    return common::Position(global_x, global_y, global_z);
  }

 private:
  // This is the position of the middle Tile (-1, -1 as we know one extra column/row to the right/bottom)
  // 18x14 tiles, so the middle Tile has index 8, 6.
  common::Position m_position = { 0, 0, 0 };
  TileArray m_tiles;
};

}  // namespace wsclient::wsworld

#endif  // WSCLIENT_SRC_TILES_H_
