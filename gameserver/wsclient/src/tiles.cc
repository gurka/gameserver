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

#include "tiles.h"

#include "logger.h"

namespace
{

int index(int x, int y, int z)
{
  return (z * wsclient::consts::KNOWN_TILES_X * wsclient::consts::KNOWN_TILES_Y) +
         (y * wsclient::consts::KNOWN_TILES_X) +
         (x);
}

}  // namespace

namespace wsclient::wsworld
{

void Tiles::shiftTiles(common::Direction direction)
{
  // Note, the direction we receive is where we received new Tiles
  // So if direction is NORTH then we received a new top row and should
  // therefore shift all Tiles down
  // If direction is WEST then we shift everything to the right, and so on
  const auto num_floors = getNumFloors();
  for (int z = 0; z < num_floors; z++)
  {
    switch (direction)
    {
      case common::Direction::NORTH:
        // Shift everything down, starting from bottom row
        for (int y = consts::KNOWN_TILES_Y - 1; y > 0; --y)
        {
          for (int x = 0; x < consts::KNOWN_TILES_X; ++x)
          {
            const auto index_from = index(x, y - 1, z);
            const auto index_to = index(x, y, z);
            std::swap(m_tiles[index_to].things, m_tiles[index_from].things);
          }
        }
        break;

      case common::Direction::EAST:
        // Shift everything left, starting from left column
        for (int x = 0; x < consts::KNOWN_TILES_X - 1; ++x)
        {
          for (int y = 0; y < consts::KNOWN_TILES_Y; ++y)
          {
            const auto index_from = index(x + 1, y, z);
            const auto index_to = index(x, y, z);
            std::swap(m_tiles[index_to].things, m_tiles[index_from].things);
          }
        }
        break;

      case common::Direction::SOUTH:
        // Shift everything up, starting from top row
        for (int y = 0; y < consts::KNOWN_TILES_Y - 1; ++y)
        {
          for (int x = 0; x < consts::KNOWN_TILES_X; ++x)
          {
            const auto index_from = index(x, y + 1, z);
            const auto index_to = index(x, y, z);
            std::swap(m_tiles[index_to].things, m_tiles[index_from].things);
          }
        }
        break;

      case common::Direction::WEST:
        // Shift everything right, starting from right column
        for (int x = consts::KNOWN_TILES_X - 1; x > 0; --x)
        {
          for (int y = 0; y < consts::KNOWN_TILES_Y; ++y)
          {
            const auto index_from = index(x - 1, y, z);
            const auto index_to = index(x, y, z);
            std::swap(m_tiles[index_to].things, m_tiles[index_from].things);
          }
        }
        break;
    }
  }
}

void Tiles::swapFloors(int z1, int z2)
{
  if (z1 == z2)
  {
    return;
  }

  const auto start_z1 = index(0, 0, z1);
  const auto start_z2 = index(0, 0, z2);
  const auto tiles_per_floor = consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y;
  for (auto i = 0; i < tiles_per_floor; ++i)
  {
    std::swap(m_tiles[start_z1 + i], m_tiles[start_z2 + i]);
  }
}

void Tiles::shiftFloorForwards(int num_floors)
{
  for (auto z = num_floors; z > 0; --z)
  {
    const auto a = index(0, 0, z);
    const auto b = index(0, 0, z - 1);
    for (auto i = 0; i < consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y; ++i)
    {
      std::swap(m_tiles[a + i], m_tiles[b + i]);
    }
  }
}

void Tiles::shiftFloorBackwards(int num_floors)
{
  for (auto z = 0; z < num_floors; ++z)
  {
    const auto a = index(0, 0, z);
    const auto b = index(0, 0, z + 1);
    for (auto i = 0; i < consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y; ++i)
    {
      std::swap(m_tiles[a + i], m_tiles[b + i]);
    }
  }
}

Tile* Tiles::getTileLocalPos(int local_x, int local_y, int local_z)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto& tile = static_cast<const Tiles*>(this)->getTileLocalPos(local_x, local_y, local_z);
  return const_cast<Tile*>(tile);
}

const Tile* Tiles::getTileLocalPos(int local_x, int local_y, int local_z) const
{
  const auto tiles_index = index(local_x, local_y, local_z);
  if (tiles_index < 0 || tiles_index >= static_cast<int>(m_tiles.size()))
  {
    LOG_ERROR("%s: position: (%d, %d) / index: %d out of bounds", __func__, local_x, local_y, tiles_index);
    return nullptr;
  }
  return &(m_tiles[tiles_index]);
}

Tile* Tiles::getTile(const common::Position& position)
{
  // According to https://stackoverflow.com/a/123995/969365
  const auto& tile = static_cast<const Tiles*>(this)->getTile(position);
  return const_cast<Tile*>(tile);
}

const Tile* Tiles::getTile(const common::Position& position) const
{
  if (!positionIsKnown(position))
  {
    LOG_ERROR("%s: we don't know position: %s", __func__, position.toString().c_str());
    return nullptr;
  }

  const auto local_pos = globalToLocalPosition(position);
  return getTileLocalPos(local_pos.getX(), local_pos.getY(), local_pos.getZ());
}

}  // namespace wsclient::wsworld
