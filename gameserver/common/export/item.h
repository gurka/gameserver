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

#ifndef COMMON_EXPORT_ITEM_H_
#define COMMON_EXPORT_ITEM_H_

#include <cstdint>
#include <array>
#include <ostream>
#include <string>
#include <vector>

namespace common
{

using ItemUniqueId = std::uint64_t;
using ItemTypeId = std::uint16_t;

struct ItemType
{
  ItemTypeId id = 0;

  enum class Type
  {
    ITEM,
    CREATURE,
    EFFECT,
    MISSILE,
  } type = Type::ITEM;

  // Flags from data file
  bool is_ground          = false;
  bool is_on_bottom       = false;
  bool is_on_top          = false;
  bool is_container       = false;
  bool is_stackable       = false;
  bool is_multi_use       = false;
  bool is_force_use       = false;
  bool is_writable        = false;
  bool is_writable_once   = false;
  bool is_fluid_container = false;
  bool is_splash          = false;
  bool is_blocking        = false;
  bool is_immovable       = false;
  bool is_missile_block   = false;
  bool is_not_pathable    = false;
  bool is_equipable       = false;
  bool is_floor_change    = false;
  bool is_full_ground     = false;
  bool is_displaced       = false;
  bool is_rotateable      = false;
  bool is_corpse          = false;
  bool is_hangable        = false;
  bool is_hook_south      = false;
  bool is_hook_east       = false;
  bool is_animate_always  = false;

  // Extra info from data file
  int speed           = 0;
  int writable_length = 0;
  int light_size      = 0;
  std::array<int, 3> light_data = {};
  int elevation       = 0;
  int minimap_color   = 0;

  std::uint8_t sprite_width         = 0U;
  std::uint8_t sprite_height        = 0U;
  std::uint8_t sprite_extra         = 0U;
  std::uint8_t sprite_blend_frames  = 0U;
  std::uint8_t sprite_xdiv          = 0U;
  std::uint8_t sprite_ydiv          = 0U;
  std::uint8_t sprite_num_anim      = 0U;
  std::vector<std::uint16_t> sprites;

  // Loaded from xml file (server only)
  std::string name      = "";
  int weight            = 0;
  int decayto           = 0;
  int decaytime         = 0;
  int damage            = 0;
  std::uint8_t maxitems = 0;
  std::string type_xml  = "";
  std::string position  = "";
  int attack            = 0;
  int defence           = 0;
  int arm               = 0;
  std::string skill     = "";
  std::string descr     = "";
  int handed            = 0;
  int shottype          = 0;
  std::string amutype   = "";

  void dump(std::ostream* os_in, bool include_server_data) const
  {
    if (!os_in)
    {
      return;
    }

    auto& os = *os_in;
    os << "Item [ ";
    os << "id="    << id << " ";
    os << "type="  << (type == Type::ITEM ? "ITEM" : (type == Type::CREATURE ? "CREATURE" : (type == Type::EFFECT ? "EFFECT" : "MISSILE"))) << " ";
    os << (is_ground          ? "is_ground "          : "");
    os << (is_on_bottom       ? "is_on_bottom "       : "");
    os << (is_on_top          ? "is_on_top "          : "");
    os << (is_container       ? "is_container "       : "");
    os << (is_stackable       ? "is_stackable "       : "");
    os << (is_multi_use       ? "is_multi_use "       : "");
    os << (is_force_use       ? "is_force_use "       : "");
    os << (is_writable        ? "is_writable "        : "");
    os << (is_writable_once   ? "is_writable_once "   : "");
    os << (is_fluid_container ? "is_fluid_container " : "");
    os << (is_splash          ? "is_splash "          : "");
    os << (is_blocking        ? "is_blocking "        : "");
    os << (is_immovable       ? "is_immovable "       : "");
    os << (is_missile_block   ? "is_missile_block "   : "");
    os << (is_not_pathable    ? "is_not_pathable "    : "");
    os << (is_equipable       ? "is_equipable "       : "");
    os << (is_floor_change    ? "is_floor_change "    : "");
    os << (is_full_ground     ? "is_full_ground "     : "");
    os << (is_displaced       ? "is_displaced "       : "");
    os << (is_rotateable      ? "is_rotateable "      : "");
    os << (is_corpse          ? "is_corpse "          : "");
    os << (is_hangable        ? "is_hangable "        : "");
    os << (is_hook_south      ? "is_hook_south "      : "");
    os << (is_hook_east       ? "is_hook_east "       : "");
    os << (is_animate_always  ? "is_animate_always "  : "");

    if (speed > 0)
    {
      os << "speed=" << speed << " ";
    }
    if (is_writable || is_writable_once)
    {
      os << "writable_length=" << writable_length << " ";
    }
    if (light_size > 0)
    {
      os << "light_size=" << light_size << " ";
      os << "light_data=" << light_data[0] << ", " << light_data[1] << ", " << light_data[2] << " ";
    }
    if (elevation > 0)
    {
      os << "elevation="     << elevation << " ";
    }
    if (minimap_color > 0)
    {
      os << "minimap_color=" << minimap_color << " ";
    }

    os << "Sprite [";
    os << "width="    << static_cast<int>(sprite_width) << " ";
    os << "height="   << static_cast<int>(sprite_height) << " ";
    os << "extra="    << static_cast<int>(sprite_extra) << " ";
    os << "blend="    << static_cast<int>(sprite_blend_frames) << " ";
    os << "xdiv="     << static_cast<int>(sprite_xdiv) << " ";
    os << "ydiv="     << static_cast<int>(sprite_ydiv) << " ";
    os << "num_anim=" << static_cast<int>(sprite_num_anim);
    os << "] ";

    os << "Sprite IDs [ ";
    for (const auto& sprite_id : sprites)
    {
      os << sprite_id << " ";
    }
    os << "]";

    if (include_server_data)
    {
      // TODO(simon): implement
    }
  }
};

class Item
{
 public:
  static constexpr ItemUniqueId INVALID_UNIQUE_ID = 0U;

  virtual ~Item() = default;

  virtual ItemUniqueId getItemUniqueId() const = 0;
  virtual ItemTypeId getItemTypeId() const = 0;

  virtual const ItemType& getItemType() const = 0;

  virtual std::uint8_t getCount() const = 0;
  virtual void setCount(std::uint8_t count) = 0;

  bool operator==(const Item& other) const
  {
    return getItemUniqueId() == other.getItemUniqueId();
  }

  bool operator!=(const Item& other) const
  {
    return !(*this == other);
  }
};

}  // namespace common

#endif  // COMMON_EXPORT_ITEM_H_
