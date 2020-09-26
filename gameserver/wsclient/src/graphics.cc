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

void drawItem(int x, int y, const common::ItemType& item_type, std::uint16_t elevation, int anim_tick)
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
  //              need to figure out how to use position, anim_tick and count/extra to determine sprite
  //              some items have count (select sprite based on if 1, 2, 3, 4, 5 or 10 (?)
  //              some items (fluid container) select sprite based on the content (extra?)
  auto* texture = getTexture(item_type.id).getItemTexture(common::Position(x, y, 0U), anim_tick);
  if (!texture)
  {
    return;
  }

  // TODO(simon): there is probably a max offset...
  const SDL_Rect dest
  {
    (x * TILE_SIZE - elevation - (item_type.is_displaced ? 8 : 0) - ((item_type.sprite_width - 1) * 32)) * SCALE,
    (y * TILE_SIZE - elevation - (item_type.is_displaced ? 8 : 0) - ((item_type.sprite_height - 1) * 32)) * SCALE,
    item_type.sprite_width * TILE_SIZE_SCALED,
    item_type.sprite_height * TILE_SIZE_SCALED
  };
  SDL_RenderCopy(sdl_renderer, texture, nullptr, &dest);
}

void drawCreature(int x, int y, const wsclient::wsworld::Creature& creature, std::uint16_t offset)
{
  if (creature.outfit.type == 0U && creature.outfit.item_id != 0U)
  {
    // note: if both are zero then creature is invis
    const auto& item_type = (*itemtypes)[creature.outfit.item_id];
    drawItem(x, y, item_type, 0, 0);
    return;
  }

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

  for (auto y = 0; y < wsclient::consts::DRAW_TILES_Y + 1; y++)
  {
    // Skip first column
    ++it;

    for (auto x = 0; x < wsclient::consts::DRAW_TILES_X + 1; x++)
    {
      const auto& tile = *it;
      ++it;

      if (tile.things.empty())
      {
        continue;
      }

      // Order:
      // 1. Bottom items (ground, on_bottom)
      // 2. Common items in reverse order (neither creature, on_bottom nor on_top)
      // 3. Creatures (reverse order?)
      // 4. (Effects)
      // 5. Top items (on_top)

      // Keep track of elevation
      auto elevation = 0;

      // Draw ground and on_bottom items
      for (const auto& thing : tile.things)
      {
        if (std::holds_alternative<wsclient::wsworld::Item>(thing))
        {
          const auto& item = std::get<wsclient::wsworld::Item>(thing);
          if (item.type->is_ground || item.type->is_on_bottom)
          {
            drawItem(x, y, *item.type, elevation, anim_tick);
            elevation += item.type->elevation;
            continue;
          }
        }
        break;
      }

      // Draw items, neither on_bottom nor on_top, in reverse order
      for (auto it = tile.things.rbegin(); it != tile.things.rend(); ++it)
      {
        if (std::holds_alternative<wsclient::wsworld::Item>(*it))
        {
          const auto& item = std::get<wsclient::wsworld::Item>(*it);
          if (!item.type->is_ground && !item.type->is_on_top && !item.type->is_on_bottom)
          {
            drawItem(x, y, *item.type, elevation, anim_tick);
            elevation += item.type->elevation;
            continue;
          }

          if (item.type->is_on_top)
          {
            // to not hit the break below as there can be items left to draw here
            continue;
          }
        }
        break;
      }

      // Draw creatures, in reverse order
      for (auto it = tile.things.rbegin(); it != tile.things.rend(); ++it)
      {
        if (std::holds_alternative<common::CreatureId>(*it))
        {
          const auto creature_id = std::get<common::CreatureId>(*it);
          const auto* creature = map.getCreature(creature_id);
          if (creature)
          {
            drawCreature(x, y, *creature, elevation);
          }
          else
          {
            LOG_ERROR("%s: cannot render creature with id %u, no creature data",
                      __func__,
                      creature_id);
          }
        }
      }

      // Draw on_top items
      for (const auto& thing : tile.things)
      {
        if (std::holds_alternative<wsclient::wsworld::Item>(thing))
        {
          const auto& item = std::get<wsclient::wsworld::Item>(thing);
          if (item.type->is_on_top)
          {
            drawItem(x, y, *item.type, elevation, anim_tick);
            elevation += item.type->elevation;
            continue;
          }
        }
        break;
      }
    }

    // Skip the 2nd extra column to the right
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

  // TODO(simon): create multiple surfaces to draw on (draw game in one, chat in one, sidebar in one) and then render them together on the screen
  //              also make game surface "known tiles" large and then draw a sub-window of it ("draw tiles") to the screen
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
    // Skip floors above player z as we don't know when floor above blocks view of player yet
    for (auto z = 0; z <= 7; ++z)
    {
      drawFloor(map, tiles.cbegin() + (z * consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y), anim_tick);
      if (7 - map.getPlayerPosition().getZ() == z)
      {
        break;
      }
    }
  }
  else
  {
    // Underground, draw below floors up to player floor
    drawFloor(map, tiles.cbegin() + (0 * consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y), anim_tick);
    drawFloor(map, tiles.cbegin() + (1 * consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y), anim_tick);
    drawFloor(map, tiles.cbegin() + (2 * consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y), anim_tick);
  }

  SDL_RenderPresent(sdl_renderer);
}

common::Position screenToMapPosition(int x, int y)
{
  return {
    static_cast<std::uint16_t>((x / TILE_SIZE_SCALED) + 1),  // adjust for draw tiles vs. known tiles
    static_cast<std::uint16_t>((y / TILE_SIZE_SCALED) + 1),  // adjust for draw tiles vs. known tiles
    0  // only map knows
  };
}

}  // namespace wsclient::graphics
