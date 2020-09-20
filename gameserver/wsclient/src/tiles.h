#ifndef WSCLIENT_SRC_TILES_H_
#define WSCLIENT_SRC_TILES_H_

#include <array>
#include <variant>

#include "position.h"
#include "item.h"
#include "creature.h"
#include "logger.h"
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

using TileArray = std::array<Tile, consts::known_tiles_x * consts::known_tiles_y * 8>;

class Tiles
{
 public:
  const common::Position& getMapPosition() const { return m_position; }
  void setMapPosition(const common::Position& position) { m_position = position; }

  void shiftTiles(common::Direction direction)
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
          for (int y = consts::known_tiles_y - 1; y > 0; --y)
          {
            for (int x = 0; x < consts::known_tiles_x; ++x)
            {
              const auto index_from = index(x, y - 1, z);
              const auto index_to = index(x, y, z);
              std::swap(m_tiles[index_to].things, m_tiles[index_from].things);
            }
          }
          break;

        case common::Direction::EAST:
          // Shift everything left, starting from left column
          for (int x = 0; x < consts::known_tiles_x - 1; ++x)
          {
            for (int y = 0; y < consts::known_tiles_y; ++y)
            {
              const auto index_from = index(x + 1, y, z);
              const auto index_to = index(x, y, z);
              std::swap(m_tiles[index_to].things, m_tiles[index_from].things);
            }
          }
          break;

        case common::Direction::SOUTH:
          // Shift everything up, starting from top row
          for (int y = 0; y < consts::known_tiles_y - 1; ++y)
          {
            for (int x = 0; x < consts::known_tiles_x; ++x)
            {
              const auto index_from = index(x, y + 1, z);
              const auto index_to = index(x, y, z);
              std::swap(m_tiles[index_to].things, m_tiles[index_from].things);
            }
          }
          break;

        case common::Direction::WEST:
          // Shift everything right, starting from right column
          for (int x = consts::known_tiles_x - 1; x > 0; --x)
          {
            for (int y = 0; y < consts::known_tiles_y; ++y)
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

  Tile* getTileLocalPos(int local_x, int local_y, int local_z)
  {
    // According to https://stackoverflow.com/a/123995/969365
    const auto& tile = static_cast<const Tiles*>(this)->getTileLocalPos(local_x, local_y, local_z);
    return const_cast<Tile*>(tile);
  }

  const Tile* getTileLocalPos(int local_x, int local_y, int local_z) const
  {
    const auto tiles_index = index(local_x, local_y, local_z);
    if (tiles_index < 0 || tiles_index >= static_cast<int>(m_tiles.size()))
    {
      LOG_ERROR("%s: position: (%d, %d) / index: %d out of bounds", __func__, local_x, local_y, tiles_index);
      return nullptr;
    }
    return &(m_tiles[tiles_index]);
  }

  Tile* getTile(const common::Position& position)
  {
    // According to https://stackoverflow.com/a/123995/969365
    const auto& tile = static_cast<const Tiles*>(this)->getTile(position);
    return const_cast<Tile*>(tile);
  }

  const Tile* getTile(const common::Position& position) const
  {
    // z mapping depends on what floor the player is on
    // see getMapData in protocol_client.cc
    const auto local_z = (m_position.getZ() <= 7) ? (7 - position.getZ()) : (position.getZ() - m_position.getZ() + 2);
    const auto local_x = position.getX() + consts::known_tiles_offset_x - m_position.getX();
    const auto local_y = position.getY() + consts::known_tiles_offset_y - m_position.getY();
    return getTileLocalPos(local_x, local_y, local_z);
  }

  const auto& getTiles() const { return m_tiles; }
  int getNumFloors() const
  {
    return  (m_position.getZ() <= 7) ? 8 :
           ((m_position.getZ() <= 13) ? 5 :
           ((m_position.getZ() <= 14) ? 4 : 3));
  }

 private:
  constexpr int index(int x, int y, int z) const
  {
    return (z * consts::known_tiles_x * consts::known_tiles_y) + (y * consts::known_tiles_x) + x;
  }

  // This is the position of the middle Tile (-1, -1 as we know one extra column/row to the right/bottom)
  // 18x14 tiles, so the middle Tile has index 8, 6.
  common::Position m_position = { 0, 0, 0 };
  TileArray m_tiles;
};

}  // namespace wsclient::wsworld

#endif  // WSCLIENT_SRC_TILES_H_
