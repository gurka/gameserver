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
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <emscripten.h>
#include <SDL.h>

#include "logger.h"
#include "data_loader.h"
#include "sprite_loader.h"
#include "texture.h"

constexpr auto data_filename = "files/data.dat";
constexpr auto sprite_filename = "files/sprite.dat";

constexpr auto screen_width = 320;
constexpr auto screen_height = 320;
constexpr auto tile_size = 32;

constexpr auto scale = 2;
constexpr auto screen_width_scaled = screen_width * scale;
constexpr auto screen_height_scaled = screen_height * scale;
constexpr auto tile_size_scaled = tile_size * scale;

SDL_Renderer* sdl_renderer = nullptr;

io::SpriteLoader sprite_loader;
io::data_loader::ItemTypes item_types;
common::ItemTypeId item_type_id_first;
common::ItemTypeId item_type_id_last;
common::ItemType item_type;
wsclient::Texture texture;

void logItem(const common::ItemType& item_type)
{
  std::ostringstream ss;
  ss << "ItemTypeId=" << item_type.id;
  ss << (item_type.ground ? " ground" : "");
  ss << " speed=" << item_type.speed;
  ss << (item_type.is_blocking ? " is_blocking" : "");
  ss << (item_type.always_on_top ? " always_on_top" : "");
  ss << (item_type.is_container ? " is_container" : "");
  ss << (item_type.is_stackable ? " is_stackable" : "");
  ss << (item_type.is_usable ? " is_usable" : "");
  ss << (item_type.is_multitype ? " is_multitype" : "");
  ss << (item_type.is_not_movable ? " is_not_movable" : "");
  ss << (item_type.is_equipable ? " is_equipable" : "");
  LOG_INFO(ss.str().c_str());

  ss.str("");
  ss << "Sprite:";
  ss << " width="    << static_cast<int>(item_type.sprite_width)
     << " height="   << static_cast<int>(item_type.sprite_height)
     << " extra="    << static_cast<int>(item_type.sprite_extra)
     << " blend="    << static_cast<int>(item_type.sprite_blend_frames)
     << " xdiv="     << static_cast<int>(item_type.sprite_xdiv)
     << " ydiv="     << static_cast<int>(item_type.sprite_ydiv)
     << " num_anim=" << static_cast<int>(item_type.sprite_num_anim);
  LOG_INFO(ss.str().c_str());

  ss.str("");
  ss << "Sprite IDs:";
  for (const auto& sprite_id : item_type.sprites)
  {
    ss << " " << sprite_id;
  }
  LOG_INFO(ss.str().c_str());

  ss.str("");
  ss << "Unknowns:";
  for (const auto& unknown : item_type.unknown_properties)
  {
    ss << " id=" << static_cast<int>(unknown.id) << ",extra=" << unknown.extra;
  }
  LOG_INFO(ss.str().c_str());
}

void setItemType(common::ItemTypeId item_type_id)
{
  item_type = item_types[item_type_id];
  texture = wsclient::Texture::create(sdl_renderer, sprite_loader, item_type);
  if (texture.getTextures().empty())
  {
    LOG_ERROR("No textures for item type id: %d", item_type_id);
  }
  logItem(item_type);
}

void render()
{
  const auto& textures = texture.getTextures();

  if (textures.empty())
  {
    return;
  }

  const auto anim_tick = SDL_GetTicks() / 540;

  SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);
  SDL_RenderClear(sdl_renderer);

  // Render all blends, xdivs and ydivs
  for (auto y = 0; y < item_type.sprite_ydiv; y++)
  {
    for (auto x = 0; x < item_type.sprite_xdiv; x++)
    {
      const auto texture_index = x + (y * item_type.sprite_xdiv) + (anim_tick % item_type.sprite_num_anim);
      if (texture_index < 0 || texture_index >= textures.size())
      {
        LOG_ERROR("%s: texture_index: %d is invalid (textures.size(): %d)",
                  __func__,
                  texture_index,
                  static_cast<int>(textures.size()));
        return;
      }

      const SDL_Rect dest
      {
        (x - item_type.sprite_width + 1) * tile_size_scaled,
        (y - item_type.sprite_height + 1) * tile_size_scaled,
        item_type.sprite_width * tile_size_scaled,
        item_type.sprite_height * tile_size_scaled
      };
      SDL_RenderCopy(sdl_renderer, textures[texture_index], nullptr, &dest);
    }
  }

  SDL_RenderPresent(sdl_renderer);
}

void handleEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event) == 1)
  {
    if (event.type == SDL_KEYUP)
    {
      if (event.key.keysym.sym == SDLK_RIGHT && item_type.id != item_type_id_last)
      {
        setItemType(item_type.id + 1);
      }
      else if (event.key.keysym.sym == SDLK_LEFT && item_type.id != item_type_id_first)
      {
        setItemType(item_type.id - 1);
      }
    }
  }
}

extern "C" void mainLoop()
{
  render();
  handleEvents();
}

int main()
{
  // Load data
  if (!io::data_loader::load(data_filename, &item_types, &item_type_id_first, &item_type_id_last))
  {
    LOG_ERROR("Could not load data");
    return 1;
  }

  // Load sprites
  if (!sprite_loader.load(sprite_filename))
  {
    LOG_ERROR("Could not load sprites");
    return 1;
  }

  // Load SDL
  SDL_Init(SDL_INIT_VIDEO);
  auto* sdl_window = SDL_CreateWindow("itemview",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      screen_width_scaled,
                                      screen_height_scaled,
                                      0);
  if (!sdl_window)
  {
    LOG_ERROR("%s: could not create window: %s", __func__, SDL_GetError());
    return 1;
  }

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  if (!sdl_renderer)
  {
    LOG_ERROR("%s: could not create renderer: %s", __func__, SDL_GetError());
    return 1;
  }

  // Load initial item type
  setItemType(item_type_id_first);

  LOG_INFO("itemview started");

  emscripten_set_main_loop(mainLoop, 0, true);
}

