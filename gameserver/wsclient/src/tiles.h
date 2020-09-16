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

class Tiles
{
 public:
  const common::Position& getMapPosition() const { return m_position; }
  void setMapPosition(const common::Position& position) { m_position = position; }

  Tile& getTile(const common::Position& position)
  {
    // According to https://stackoverflow.com/a/123995/969365
    const auto& tile = static_cast<const Tiles*>(this)->getTile(position);
    return const_cast<Tile&>(tile);
  }

  const Tile& getTile(const common::Position& position) const
  {
    const auto local_x = position.getX() + consts::known_tiles_offset_x - m_position.getX();
    const auto local_y = position.getY() + consts::known_tiles_offset_y - m_position.getY();
    const auto index = (local_y * consts::known_tiles_x) + local_x;
    if (index < 0 || index >= m_tiles.size())
    {
      LOG_ERROR("%s: position: (%d, %d) / index: %d out of bounds", __func__, local_x, local_y, index);
    }
    return m_tiles[index];
  }

  const auto& getTiles() const { return m_tiles; }

 private:
  // This is the position of the middle Tile (-1, -1 as we know one extra column/row to the right/bottom)
  // 18x14 tiles, so the middle Tile has index 8, 6.
  common::Position m_position = { 0, 0, 0 };
  std::array<Tile, consts::known_tiles_x * consts::known_tiles_y> m_tiles;
};

}  // namespace wsclient::wsworld

#endif  // WSCLIENT_SRC_TILES_H_
