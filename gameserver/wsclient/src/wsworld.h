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
#ifndef WSCLIENT_SRC_WSWORLD_H_
#define WSCLIENT_SRC_WSWORLD_H_

#include <array>
#include <vector>

#include "position.h"
#include "creature.h"
#include "item.h"
#include "protocol_types.h"

#include "types.h"

namespace wsclient::wsworld
{

using ::world::Position;
using ::world::CreatureId;
using ::world::ItemTypeId;

struct ItemType
{
  ItemTypeId id       = 0U;

  bool ground         = false;
  std::uint8_t speed  = 0U;
  bool is_blocking    = false;
  bool always_on_top  = false;
  bool is_container   = false;
  bool is_stackable   = false;
  bool is_usable      = false;
  bool is_multitype   = false;
  bool is_not_movable = false;
  bool is_equipable   = false;

  std::uint8_t sprite_width  = 0U;
  std::uint8_t sprite_height = 0U;
  std::uint8_t sprite_extra  = 0U;
  std::uint8_t blend_frames  = 0U;
  std::uint8_t xdiv          = 0U;
  std::uint8_t ydiv          = 0U;
  std::uint8_t num_anim      = 0U;
  std::vector<std::uint16_t> sprites;
};

using ItemTypes = std::array<ItemType, 4096>;

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

}  // namespace wsclient::wsworld

#endif  // WSCLIENT_SRC_WSWORLD_H_
