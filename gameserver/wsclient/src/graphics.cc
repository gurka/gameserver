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
#include <array>
#include <iterator>
#include <tuple>
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

//        map width       sidebar width
//  _______________________________
// |                        |      |
// |                        |      |
// |                  map   |      |
// |                 height |      |
// |                        |      |  sidebar
// |                        |      |  height
// |     chat width         |      |
// |________________________|      |
// |                  chat  |      |
// |                  height|      |
// |________________________|______|
//
// Map width is constant
// Map height is constant
// Sidebar width is dynamic
// Sidebar height = map height + chat height
// Chat width = map width
// Chat height is dynamic
//
// Screen size: map width + sidebar width, sidebar height
//

constexpr auto TILE_SIZE = 32;
constexpr auto MAP_TEXTURE_WIDTH  = wsclient::consts::DRAW_TILES_X * TILE_SIZE;  // 480
constexpr auto MAP_TEXTURE_HEIGHT = wsclient::consts::DRAW_TILES_Y * TILE_SIZE;  // 352

constexpr auto CHAT_TEXTURE_WIDTH = MAP_TEXTURE_WIDTH;  // 480
constexpr auto CHAT_TEXTURE_HEIGHT = 128;

constexpr auto SIDEBAR_TEXTURE_WIDTH = 160;
constexpr auto SIDEBAR_TEXTURE_HEIGHT = MAP_TEXTURE_HEIGHT + CHAT_TEXTURE_HEIGHT;  // 480

constexpr auto SCREEN_WIDTH_NO_SCALING = MAP_TEXTURE_WIDTH + SIDEBAR_TEXTURE_WIDTH;  // 640
constexpr auto SCREEN_HEIGHT_NO_SCALING = SIDEBAR_TEXTURE_HEIGHT;  // 480

double current_width_scale = 1.0f;
double current_height_scale = 1.0f;

SDL_Window* sdl_window = nullptr;
SDL_Renderer* sdl_renderer = nullptr;
SDL_Texture* sdl_map_texture = nullptr;
SDL_Texture* sdl_chat_texture = nullptr;
SDL_Texture* sdl_sidebar_texture = nullptr;

const utils::data_loader::ItemTypes* itemtypes = nullptr;
wsclient::SpriteLoader sprite_loader;

struct CreatureTexture
{
  common::CreatureId creature_id;
  wsclient::Texture texture;
};
std::vector<CreatureTexture> creature_textures;
std::vector<wsclient::Texture> item_textures;

enum class HangableHookSide
{
  NONE,
  SOUTH,
  EAST,
};

const wsclient::Texture* getCreatureTexture(common::CreatureId creature_id)
{
  auto it = std::find_if(creature_textures.cbegin(),
                         creature_textures.cend(),
                         [&creature_id](const CreatureTexture& creature_texture)
  {
    return creature_texture.creature_id == creature_id;
  });
  if (it == creature_textures.cend())
  {
    LOG_ERROR("%s: could not find CreatureTexture for CreatureId: %u", __func__, creature_id);
    return nullptr;
  }
  return &(it->texture);
}

const wsclient::Texture& getItemTexture(common::ItemTypeId item_type_id)
{
  auto it = std::find_if(item_textures.cbegin(),
                         item_textures.cend(),
                         [&item_type_id](const wsclient::Texture& item_texture)
  {
    return item_texture.getItemTypeId() == item_type_id;
  });

  // Create textures if not found
  if (it == item_textures.cend())
  {
    const auto& item_type = (*itemtypes)[item_type_id];
    item_textures.push_back(wsclient::Texture::createItemTexture(sdl_renderer,
                                                                 sprite_loader,
                                                                 item_type));
    it = item_textures.end() - 1;
  }

  return *it;
}

SDL_Texture* getItemSDLTexture(int x, int y, const wsclient::wsworld::Item& item, HangableHookSide hook_side, int anim_tick)
{

  const auto& texture = getItemTexture(item.type->id);

  auto version = 0;
  if ((item.type->is_fluid_container || item.type->is_splash) && item.extra < texture.getNumVersions())
  {
    version = item.extra;
  }
  else if (item.type->is_stackable)
  {
    // index 0 -> count 1
    // index 1 -> count 2
    // index 2 -> count 3
    // index 3 -> count 4
    // index 4 -> count 5
    // index 5 -> count 6..10 (?)
    // index 6 -> count 11..25 (?)
    // index 7 -> count 25..100 (?)
    if (item.extra <= 5U)
    {
      version = item.extra - 1;
    }
    else if (item.extra <= 10U)
    {
      version = 5;
    }
    else if (item.extra <= 25U)
    {
      version = 6;
    }
    else
    {
      version = 7;
    }

    // Some item have less than 8 sprites for different stack/count
    version = std::min(version, texture.getNumVersions() - 1);
  }
  else if (item.type->is_hangable && texture.getNumVersions() == 3)
  {
    switch (hook_side)
    {
      case HangableHookSide::NONE:
        version = 0;
        break;

      case HangableHookSide::SOUTH:
        version = 1;
        break;

      case HangableHookSide::EAST:
        version = 2;
        break;
    }
  }
  else
  {
    // TODO(simon): need to use world position, not local position
    version = ((y % item.type->sprite_info.ydiv) * item.type->sprite_info.xdiv) + (x % item.type->sprite_info.xdiv);
  }

  return texture.getItemTexture(version, anim_tick);
}

void drawItem(int x, int y, const wsclient::wsworld::Item& item, HangableHookSide hook_side, std::uint16_t elevation, int anim_tick)
{
  if (item.type->type != common::ItemType::Type::ITEM)
  {
    LOG_ERROR("%s: called but item type: %u is not an item", __func__, item.type->id);
    return;
  }

  if (item.type->id == 0)
  {
    return;
  }

  auto* sdl_texture = getItemSDLTexture(x, y, item, hook_side, anim_tick);

  // TODO(simon): there is probably a max offset...
  const SDL_Rect dest
  {
    (x * TILE_SIZE - elevation - (item.type->is_displaced ? 8 : 0) - ((item.type->sprite_info.width - 1) * 32)),
    (y * TILE_SIZE - elevation - (item.type->is_displaced ? 8 : 0) - ((item.type->sprite_info.height - 1) * 32)),
    item.type->sprite_info.width * TILE_SIZE,
    item.type->sprite_info.height * TILE_SIZE
  };
  SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, &dest);
}

void drawCreature(int x, int y, const wsclient::wsworld::Creature& creature, std::uint16_t offset)
{
  if (creature.outfit.type == 0U)
  {
    // note: if both are zero then creature is invis
    if (creature.outfit.item_id != 0U)
    {
      const auto& item_type = (*itemtypes)[creature.outfit.item_id];
      wsclient::wsworld::Item item = { &item_type, 0U };
      drawItem(x, y, item, HangableHookSide::NONE, 0, 0);
    }
    return;
  }

  const auto* texture = getCreatureTexture(creature.id);
  if (!texture)
  {
    return;
  }
  auto* sdl_texture = texture->getCreatureStillTexture(creature.direction);
  if (!sdl_texture)
  {
    return;
  }

  const SDL_Rect dest
  {
    (x * TILE_SIZE - offset - 8),
    (y * TILE_SIZE - offset - 8),
    TILE_SIZE,
    TILE_SIZE
  };
  SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, &dest);
}

void drawTile(int x,
              int y,
              const wsclient::wsworld::Map& map,
              const wsclient::wsworld::Tile& tile,
              std::uint32_t anim_tick)
{
  if (tile.things.empty())
  {
    return;
  }

  // Set hangable hook side
  auto hook_side = HangableHookSide::NONE;
  for (const auto& thing : tile.things)
  {
    if (std::holds_alternative<wsclient::wsworld::Item>(thing))
    {
      const auto& item = std::get<wsclient::wsworld::Item>(thing);
      if (item.type->is_hook_east)
      {
        hook_side = HangableHookSide::EAST;
        break;
      }

      if (item.type->is_hook_south)
      {
        hook_side = HangableHookSide::SOUTH;
        break;
      }
    }
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
        drawItem(x, y, item, hook_side, elevation, anim_tick);
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
        drawItem(x, y, item, hook_side, elevation, anim_tick);
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
        drawItem(x, y, item, hook_side, elevation, anim_tick);
        elevation += item.type->elevation;
        continue;
      }
    }
    break;
  }
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
      drawTile(x, y, map, *it, anim_tick);
      ++it;
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

  current_width_scale = 1.5f;
  current_height_scale = 1.5f;

  // Create the window initially as maximized
  sdl_window = SDL_CreateWindow("wsclient",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                SCREEN_WIDTH_NO_SCALING * current_width_scale,
                                SCREEN_HEIGHT_NO_SCALING * current_height_scale,
                                SDL_WINDOW_RESIZABLE);
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

  sdl_map_texture = SDL_CreateTexture(sdl_renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET,
                                      MAP_TEXTURE_WIDTH,
                                      MAP_TEXTURE_HEIGHT);
  if (!sdl_map_texture)
  {
    LOG_ERROR("%s: could not create map texture: %s", __func__, SDL_GetError());
    return false;
  }

  sdl_chat_texture = SDL_CreateTexture(sdl_renderer,
                                       SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET,
                                       CHAT_TEXTURE_WIDTH,
                                       CHAT_TEXTURE_HEIGHT);
  if (!sdl_chat_texture)
  {
    LOG_ERROR("%s: could not create chat texture: %s", __func__, SDL_GetError());
    return false;
  }

  sdl_sidebar_texture = SDL_CreateTexture(sdl_renderer,
                                          SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_TARGET,
                                          SIDEBAR_TEXTURE_WIDTH,
                                          SIDEBAR_TEXTURE_HEIGHT);
  if (!sdl_sidebar_texture)
  {
    LOG_ERROR("%s: could not create sidebar texture: %s", __func__, SDL_GetError());
    return false;
  }

  if (!sprite_loader.load(sprite_filename))
  {
    LOG_ERROR("%s: could not sprites", __func__);
    return false;
  }

  return true;
}

void setWindowSize(int width, int height)
{
  LOG_INFO("%s: setWindowSize(%d, %d)", __func__, width, height);
  current_width_scale = static_cast<double>(width) / SCREEN_WIDTH_NO_SCALING;
  current_height_scale = static_cast<double>(height) / SCREEN_HEIGHT_NO_SCALING;
}

void draw(const wsworld::Map& map, const PlayerInfo& player_info)
{
  const auto anim_tick = SDL_GetTicks() / 540;

  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);

  // Render chat
  {
    SDL_SetRenderTarget(sdl_renderer, sdl_chat_texture);
    SDL_SetRenderDrawColor(sdl_renderer, 127, 127, 127, 255);
    SDL_RenderClear(sdl_renderer);
  }

  // Render sidebar
  {
    SDL_SetRenderTarget(sdl_renderer, sdl_sidebar_texture);
    SDL_SetRenderDrawColor(sdl_renderer, 170, 99, 93, 255);
    SDL_RenderClear(sdl_renderer);
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);

    for (auto i = 0; i < 10; ++i)
    {
      const auto& item = player_info.equipment[i];
      if (!item.type)
      {
        continue;
      }

      // SIDEBAR_TEXTURE_WIDTH = 160;
      // each item is 32x32
      // 3x32=96 pixels for items, 64 pixels for spacing
      // 24p spacing, item, 8p spacing, item, 8p spacing, item, 24p spacing
      static const std::array<std::tuple<int, int>, 10> offset_xy =
      {
        std::make_tuple(          24 + 32 + 8,                            15 ),  // head
        std::make_tuple(                   24,                       15 + 15 ),  // amulet
        std::make_tuple( 24 + 32 + 8 + 32 + 8,                       15 + 15 ),  // bag
        std::make_tuple(          24 + 32 + 8,                   15 + 32 + 8 ),  // armor
        std::make_tuple( 24 + 32 + 8 + 32 + 8,              15 + 15 + 32 + 8 ),  // shield
        std::make_tuple(                   24,              15 + 15 + 32 + 8 ),  // weapon
        std::make_tuple(          24 + 32 + 8,          15 + 32 + 8 + 32 + 8 ),  // legs
        std::make_tuple(          24 + 32 + 8, 15 + 32 + 8 + 32 + 8 + 32 + 8 ),  // feet
        std::make_tuple(                   24,     15 + 15 + 32 + 8 + 32 + 8 ),  // ring
        std::make_tuple( 24 + 32 + 8 + 32 + 8,     15 + 15 + 32 + 8 + 32 + 8 ),  // ammo
      };
      const auto x = std::get<0>(offset_xy[i]);
      const auto y = std::get<1>(offset_xy[i]);

      auto* sdl_texture = getItemSDLTexture(x, y, item, HangableHookSide::NONE, anim_tick);

      const SDL_Rect dest
      {
        x,
        y,
        TILE_SIZE,
        TILE_SIZE
      };
      SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, &dest);
    }
  }

  // Render map
  SDL_SetRenderTarget(sdl_renderer, sdl_map_texture);
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);
  if (map.ready())
  {
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
      // Underground, draw at the bottom floor up to player floor (which is always local z=2)
      for (auto z = map.getNumFloors() - 1; z >= 2; --z)
      {
        drawFloor(map, tiles.cbegin() + (z * consts::KNOWN_TILES_X * consts::KNOWN_TILES_Y), anim_tick);
      }
    }
  }

  // Finally render everything to screen
  SDL_SetRenderTarget(sdl_renderer, nullptr);

  // Chat
  const SDL_Rect dest_chat =
  {
    0,
    static_cast<int>(MAP_TEXTURE_HEIGHT * current_height_scale),
    static_cast<int>(CHAT_TEXTURE_WIDTH * current_width_scale),
    static_cast<int>(CHAT_TEXTURE_HEIGHT * current_height_scale)
  };
  SDL_RenderCopy(sdl_renderer, sdl_chat_texture, nullptr, &dest_chat);

  // Sidebar
  const SDL_Rect dest_sidebar =
  {
    static_cast<int>(MAP_TEXTURE_WIDTH * current_width_scale),
    0,
    static_cast<int>(SIDEBAR_TEXTURE_WIDTH * current_width_scale),
    static_cast<int>(SIDEBAR_TEXTURE_HEIGHT * current_height_scale)
  };
  SDL_RenderCopy(sdl_renderer, sdl_sidebar_texture, nullptr, &dest_sidebar);

  // Map
  // Note: we always want to keep 1:1 ratio, so there is some special handling
  //       if there is a difference in width and height scaling
  // TODO(simon): fix source/dest spacing issues
  const auto spacing = static_cast<int>((MAP_TEXTURE_WIDTH * current_width_scale) -
                                        (MAP_TEXTURE_WIDTH * current_height_scale));
  const SDL_Rect source_map =
  {
    spacing < 0 ? (-spacing / 2) : 0,
    0,
    MAP_TEXTURE_WIDTH - (spacing < 0 ? -spacing : 0),
    MAP_TEXTURE_HEIGHT
  };
  const SDL_Rect dest_map =
  {
    spacing / 2,
    0,
    static_cast<int>(MAP_TEXTURE_WIDTH * current_height_scale) - (spacing < 0 ? -spacing : 0),
    static_cast<int>(MAP_TEXTURE_HEIGHT * current_height_scale)
  };
  SDL_RenderCopy(sdl_renderer, sdl_map_texture, &source_map, &dest_map);

  SDL_RenderPresent(sdl_renderer);
}

void createCreatureTexture(const wsworld::Creature& creature)
{
  // TODO(simon): fix hardcoded value
  const auto& item_type = (*itemtypes)[3134 + creature.outfit.type];
  creature_textures.emplace_back();
  creature_textures.back().creature_id = creature.id;
  creature_textures.back().texture = Texture::createOutfitTexture(sdl_renderer, sprite_loader, item_type, creature.outfit);
}

void removeCreatureTexture(const wsworld::Creature& creature)
{
  auto it = std::find_if(creature_textures.begin(),
                         creature_textures.end(),
                         [creature_id = creature.id](const CreatureTexture& creature_texture)
  {
    return creature_id == creature_texture.creature_id;
  });
  if (it == creature_textures.end())
  {
    LOG_ERROR("%s: could not find Creature Texture to remove", __func__);
    return;
  }
  creature_textures.erase(it);
}

common::Position screenToMapPosition(int x, int y)
{
  // TODO(simon): fix
  return {
    static_cast<std::uint16_t>((x / TILE_SIZE) + 1),  // adjust for draw tiles vs. known tiles
    static_cast<std::uint16_t>((y / TILE_SIZE) + 1),  // adjust for draw tiles vs. known tiles
    0  // only map knows
  };
}

}  // namespace wsclient::graphics
