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
#include "map.h"

#include "logger.h"

void Map::setMapData(const ProtocolTypes::MapData& mapData)
{
  playerPosition_ = mapData.position;
  auto it = mapData.tiles.begin();
  for (auto x = 0; x < types::known_tiles_x; x++)
  {
    for (auto y = 0; y < types::known_tiles_y; y++)
    {
      tiles_[y][x].things.clear();

      if (!it->skip)
      {
        // Find largest stackpos
        const auto maxStackpos = std::max(it->items.empty() ? 0 : it->items.back().stackpos,
                                          it->creatures.empty() ? 0 : it->creatures.back().stackpos);

        // Make things vector this big
        tiles_[y][x].things.resize(maxStackpos + 1);

        // Add items
        for (const auto& item : it->items)
        {
          LOG_INFO("Adding an item at stackpos=%d with itemTypeId=%d", item.stackpos, item.item.itemTypeId);
          tiles_[y][x].things[item.stackpos].isItem = true;
          tiles_[y][x].things[item.stackpos].item.itemTypeId = item.item.itemTypeId;
          tiles_[y][x].things[item.stackpos].item.extra = item.item.extra;
          tiles_[y][x].things[item.stackpos].item.onTop = false;  // TODO fix
        }

        // Add creatures
        for (const auto& creature : it->creatures)
        {
          LOG_INFO("Adding a creature at stackpos=%d with creatureId=%d", creature.stackpos, creature.creature.id);
          tiles_[y][x].things[creature.stackpos].isItem = false;
          tiles_[y][x].things[creature.stackpos].creatureId = creature.creature.id;
        }
      }
      ++it;
    }
  }
}

void Map::addCreature(const Position& position, CreatureId creatureId)
{
  const auto x = position.getX() + 8 - playerPosition_.getX();
  const auto y = position.getY() + 6 - playerPosition_.getY();

  // Find first creature, bottom item or end
  auto it = tiles_[y][x].things.cbegin() + 1;
  while (it != tiles_[y][x].things.cend())
  {
    if (!it->isItem || !it->item.onTop)
    {
      break;
    }
    ++it;
  }

  // Add creature here
  Tile::Thing thing;
  thing.isItem = false;
  thing.creatureId = creatureId;
  it = tiles_[y][x].things.insert(it, thing);

  LOG_INFO("%s: added creatureId=%d on position=%s stackpos=%d",
           __func__,
           creatureId,
           position.toString().c_str(),
           std::distance(tiles_[y][x].things.cbegin(), it));
}

void Map::addItem(const Position& position, ItemTypeId itemTypeId, std::uint8_t extra, bool onTop)
{
  const auto x = position.getX() + 8 - playerPosition_.getX();
  const auto y = position.getY() + 6 - playerPosition_.getY();

  auto it = tiles_[y][x].things.cbegin() + 1;
  if (onTop)
  {
    // Insert after ground item
    ++it;
  }
  else
  {
    // Find first bottom item or end
    auto it = tiles_[y][x].things.cbegin();
    while (it != tiles_[y][x].things.cend())
    {
      if (it->isItem && !it->item.onTop)
      {
        break;
      }
      ++it;
    }
  }

  // Add item here
  Tile::Thing thing;
  thing.isItem = true;
  thing.item.itemTypeId = itemTypeId;
  thing.item.extra = extra;
  thing.item.onTop = onTop;
  it = tiles_[y][x].things.insert(it, thing);

  LOG_INFO("%s: added itemTypeId=%d on position=%s stackpos=%d",
           __func__,
           itemTypeId,
           position.toString().c_str(),
           std::distance(tiles_[y][x].things.cbegin(), it));
}

void Map::removeThing(const Position& position, std::uint8_t stackpos)
{
  const auto x = position.getX() + 8 - playerPosition_.getX();
  const auto y = position.getY() + 6 - playerPosition_.getY();

  tiles_[y][x].things.erase(tiles_[y][x].things.cbegin() + stackpos);

  LOG_INFO("%s: removed thing from position=%s stackpos=%d",
           __func__,
           position.toString().c_str(),
           stackpos);
}

const Map::Tile& Map::getTile(const Position& position) const
{
  const auto x = position.getX() + 8 - playerPosition_.getX();
  const auto y = position.getY() + 6 - playerPosition_.getY();

  return tiles_[y][x];
}
