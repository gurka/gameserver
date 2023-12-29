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

#ifndef WSCLIENT_SRC_UI_GAME_H_
#define WSCLIENT_SRC_UI_GAME_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>

#include "utils/data_loader.h"

#include "tiles.h"
#include "sprite_loader.h"
#include "texture.h"

namespace game
{

class Game;

class GameUI
{
 public:
  static constexpr auto TILE_SIZE = 32;
  static constexpr auto DRAW_TILES_X = 15;
  static constexpr auto DRAW_TILES_Y = 11;
  static constexpr auto TEXTURE_WIDTH  = DRAW_TILES_X * TILE_SIZE;  // 480
  static constexpr auto TEXTURE_HEIGHT = DRAW_TILES_Y * TILE_SIZE;  // 352

  GameUI(const game::Game* game,
         SDL_Renderer* renderer,
         const SpriteLoader* sprite_loader,
         const utils::data_loader::ItemTypes* item_types);

  SDL_Texture* render();

 private:
  enum class HangableHookSide
  {
    NONE,
    SOUTH,
    EAST,
  };

  void renderFloor(game::TileArray::const_iterator it);
  void renderTile(int x, int y, const game::Tile& tile);
  void renderItem(int x, int y, const game::Item& item, HangableHookSide hook_side, std::uint16_t elevation);
  void renderCreature(int x, int y, const game::Creature& creature, std::uint16_t offset);
  SDL_Texture* getItemSDLTexture(int x, int y, const game::Item& item, HangableHookSide hook_side);
  const Texture& getItemTexture(common::ItemTypeId item_type_id);
  const Texture* getCreatureTexture(common::CreatureId creature_id);

  const game::Game* m_game;
  SDL_Renderer* m_renderer;
  const SpriteLoader* m_sprite_loader;
  const utils::data_loader::ItemTypes* m_item_types;

  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> m_texture;

  std::uint32_t m_anim_tick;

  struct CreatureTexture
  {
    common::CreatureId creature_id;
    Texture texture;
  };
  std::vector<CreatureTexture> m_creature_textures;
  std::vector<Texture> m_item_textures;
};

}  // namespace game

#endif  // WSCLIENT_SRC_UI_GAME_H_
