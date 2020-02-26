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

  // Loaded from data file
  bool ground         = false;
  int  speed          = 0;
  bool is_blocking    = false;
  bool always_on_top  = false;
  bool is_container   = false;
  bool is_stackable   = false;
  bool is_usable      = false;
  bool is_multitype   = false;
  bool is_not_movable = false;
  bool is_equipable   = false;

  std::uint16_t offset = 0U;

  std::uint8_t sprite_width         = 0U;
  std::uint8_t sprite_height        = 0U;
  std::uint8_t sprite_extra         = 0U;
  std::uint8_t sprite_blend_frames  = 0U;
  std::uint8_t sprite_xdiv          = 0U;
  std::uint8_t sprite_ydiv          = 0U;
  std::uint8_t sprite_num_anim      = 0U;
  std::vector<std::uint16_t> sprites;

  // List of unknown properties read from
  // data file
  struct UnknownProp
  {
    explicit UnknownProp(std::uint8_t id)
        : id(id)
    {
    }

    UnknownProp(std::uint8_t id, std::uint16_t extra)
        : id(id),
          extra(extra)
    {
    }

    std::uint8_t  id    = 0U;
    std::uint16_t extra = 0U;
  };
  std::vector<UnknownProp> unknown_properties;

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
