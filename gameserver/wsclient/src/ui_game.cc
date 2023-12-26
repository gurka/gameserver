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

#include "ui_game.h"

#include <SDL2/SDL.h>

#include "game/game.h"

namespace ui
{

constexpr auto TILE_SIZE = 32;
constexpr auto DRAW_TILES_X = 15;
constexpr auto DRAW_TILES_Y = 11;
constexpr auto MAP_TEXTURE_WIDTH  = DRAW_TILES_X * TILE_SIZE;  // 480
constexpr auto MAP_TEXTURE_HEIGHT = DRAW_TILES_Y * TILE_SIZE;  // 352

Game::Game(const game::Game* game)
    : m_game(game),
      m_width(0),
      m_height(0),
      m_base_texture(nullptr, SDL_DestroyTexture),
      m_resized_texture(nullptr, SDL_DestroyTexture)
{
}

void Game::init(SDL_Renderer* sdl_renderer, int width, int height)
{
  m_renderer = sdl_renderer;
  onResize(width, height);
  m_base_texture.reset(SDL_CreateTexture(m_renderer,
                                         SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET,
                                         MAP_TEXTURE_WIDTH,
                                         MAP_TEXTURE_HEIGHT));
}

void Game::onResize(int width, int height)
{
  m_width = width;
  m_height = height;
  m_resized_texture.reset(SDL_CreateTexture(m_renderer,
                                            SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_TARGET,
                                            m_width,
                                            m_height));
}

void Game::onClick(int x, int y)
{

}

SDL_Texture* Game::render()
{
  m_game->getPlayerId();
  return nullptr;
}

}