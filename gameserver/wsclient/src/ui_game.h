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

#include <memory>
#include <SDL2/SDL.h>
#include "ui_widget.h"

class SDL_Renderer;
class SDL_Texture;

namespace game
{
class Game;
}

namespace ui
{

class Game : public Widget
{
 public:
  Game(const game::Game* game);
  void init(SDL_Renderer* sdl_renderer, int width, int height) override;
  void onResize(int width, int height) override;
  void onClick(int x, int y) override;
  SDL_Texture* render() override;

 private:
  const game::Game* m_game;
  SDL_Renderer* m_renderer;
  int m_width;
  int m_height;

  using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

  TexturePtr m_base_texture;
  TexturePtr m_resized_texture;
};

}

#endif  // WSCLIENT_SRC_UI_GAME_H_
