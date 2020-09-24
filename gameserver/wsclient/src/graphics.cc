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

#include "graphics.h"

#include <cstdint>

#include <algorithm>
#include <iterator>
#include <variant>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "logger.h"
#include "data_loader.h"
#include "sprite_loader.h"
#include "texture.h"
#include "tiles.h"

namespace
{

constexpr auto TILE_SIZE = 32;
constexpr auto SCALE = 2;
constexpr auto TILE_SIZE_SCALED = TILE_SIZE * SCALE;

constexpr auto SCREEN_WIDTH  = wsclient::consts::DRAW_TILES_X * TILE_SIZE_SCALED;
constexpr auto SCREEN_HEIGHT = wsclient::consts::DRAW_TILES_Y * TILE_SIZE_SCALED;

SDL_Window* sdl_window = nullptr;
SDL_Renderer* sdl_renderer = nullptr;
const utils::data_loader::ItemTypes* itemtypes = nullptr;
wsclient::SpriteLoader sprite_loader;
std::vector<wsclient::Texture> item_textures;

const wsclient::Texture& getTexture(common::ItemTypeId item_type_id)
{
  auto it = std::find_if(item_textures.cbegin(),
                         item_textures.cend(),
                         [&item_type_id](const wsclient::Texture& item_texture)
  {
    return item_texture.getItemTypeId() == item_type_id;
  });

  // Create textures if not found
  if (it == item_textures.end())
  {
    const auto& item_type = (*itemtypes)[item_type_id];
    item_textures.push_back(wsclient::Texture::create(sdl_renderer,
                                                      sprite_loader,
                                                      item_type));
    it = item_textures.end() - 1;
  }

  return *it;
}

void drawItem(int x, int y, const common::ItemType& item_type, std::uint16_t offset, int anim_tick)
{
  if (item_type.type != common::ItemType::Type::ITEM)
  {
    LOG_ERROR("%s: called but item type: %u is not an item", __func__, item_type.id);
    return;
  }

  if (item_type.id == 0)
  {
    return;
  }

  // TODO(simon): need to use world position, not local position
  auto* texture = getTexture(item_type.id).getItemTexture(common::Position(x, y, 0U), anim_tick);
  if (!texture)
  {
    return;
  }

  // TODO(simon): there is probably a max offset...
  const SDL_Rect dest
  {
    (x * TILE_SIZE - offset - ((item_type.sprite_width - 1) * 32)) * SCALE,
    (y * TILE_SIZE - offset - ((item_type.sprite_height - 1) * 32)) * SCALE,
    item_type.sprite_width * TILE_SIZE_SCALED,
    item_type.sprite_height * TILE_SIZE_SCALED
  };
  SDL_RenderCopy(sdl_renderer, texture, nullptr, &dest);
}

void drawCreature(int x, int y, const wsclient::wsworld::Creature& creature, std::uint16_t offset)
{
  // TODO(simon): fix this
  //              DataLoader need to separate what it loads into Items, Outfits, Effects and Missiles
  //              since the ids are relative
  const auto item_type_id = creature.outfit.type + 3034 + 100;

  auto* texture = getTexture(item_type_id).getCreatureStillTexture(creature.direction);
  if (!texture)
  {
    return;
  }

  const SDL_Rect dest
  {
    (x * TILE_SIZE - offset - 8) * SCALE,
    (y * TILE_SIZE - offset - 8) * SCALE,
    TILE_SIZE_SCALED,
    TILE_SIZE_SCALED
  };
  SDL_RenderCopy(sdl_renderer, texture, nullptr, &dest);
}


void drawFloor(const wsclient::wsworld::Map& map,
               wsclient::wsworld::TileArray::const_iterator it,
               std::uint32_t anim_tick)
{
  // Skip first row
  it += wsclient::consts::KNOWN_TILES_X;

  for (auto y = 0; y < wsclient::consts::DRAW_TILES_Y; y++)
  {
    // Skip first column
    ++it;

    for (auto x = 0; x < wsclient::consts::DRAW_TILES_X; x++)
    {
      const auto& tile = *it;
      ++it;

      if (tile.things.empty())
      {
        continue;
      }

      // Draw ground
      if (!std::holds_alternative<wsclient::wsworld::Item>(tile.things.front()))
      {
        LOG_ERROR("%s: first Thing on tile is not an Item!", __func__);
        return;
      }
      const auto& ground_item = std::get<wsclient::wsworld::Item>(tile.things.front());
      drawItem(x, y, *ground_item.type, 0, anim_tick);

      // Draw things in reverse order, except ground
      auto offset = ground_item.type->offset;
      for (auto it = tile.things.rbegin(); it != tile.things.rend() - 1; ++it)
      {
        const auto& thing = *it;
        if (std::holds_alternative<wsclient::wsworld::Item>(thing))
        {
          // TODO(simon): probably need things like count later
          const auto& item = std::get<wsclient::wsworld::Item>(thing);
          drawItem(x, y, *item.type, offset, anim_tick);

          offset += item.type->offset;
        }
        else if (std::holds_alternative<common::CreatureId>(thing))
        {
          const auto& creature_id = std::get<common::CreatureId>(thing);
          const auto* creature = map.getCreature(creature_id);
          if (creature)
          {
            drawCreature(x, y, *creature, offset);
          }
          else
          {
            LOG_ERROR("%s: cannot render creature with id %u, no creature data",
                      __func__,
                      creature_id);
          }
        }
        else
        {
          LOG_ERROR("%s: unknown Thing on local position: (%d, %d)", __func__, x, y);
        }
      }
    }

    // Skip the two extra columns to the right
    ++it;
    ++it;
  }
}

}  // namespace

namespace wsclient::graphics
{

bool init(const utils::data_loader::ItemTypes* itemtypes_in, const std::string& sprite_filename)
{
  itemtypes = itemtypes_in;

  SDL_Init(SDL_INIT_VIDEO);

  sdl_window = SDL_CreateWindow("wsclient", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
  if (!sdl_window)
  {
    LOG_ERROR("%s: could not create window: %s", __func__, SDL_GetError());
    return false;
  }

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  if (!sdl_renderer)
  {
    LOG_ERROR("%s: could not create renderer: %s", __func__, SDL_GetError());
    return false;
  }

  if (!sprite_loader.load(sprite_filename))
  {
    LOG_ERROR("%s: could not sprites", __func__);
    return false;
  }

  return true;
}

void draw(const wsworld::Map& map)
{
  const auto anim_tick = SDL_GetTicks() / 540;

  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);

  if (!map.ready())
  {
    SDL_RenderPresent(sdl_renderer);
    return;
  }

  const auto& tiles = map.getTiles();
  if (map.getPlayerPosition().getZ() <= 7)
  {
    // We have floors 7  6  5  4  3  2  1  0
    // and we want to draw them in that order
    for (auto z = 0; z <= 7; ++z)
    {
      drawFloor(map, tiles.cbegin() + (z * consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y), anim_tick);
    }
  }
  else
  {
    // Underground, just draw current floor
    const auto z = map.getPlayerPosition().getZ();
    drawFloor(map, tiles.cbegin() + (z * consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y), anim_tick);
  }

  SDL_RenderPresent(sdl_renderer);
}

}  // namespace wsclient::graphics
