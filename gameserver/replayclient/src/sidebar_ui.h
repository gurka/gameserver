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

#ifndef WSCLIENT_SRC_SIDEBAR_SIDEBAR_UI_H_
#define WSCLIENT_SRC_SIDEBAR_SIDEBAR_UI_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace sidebar
{

class Sidebar;

class SidebarUI
{
 public:
  struct Callbacks
  {
    std::function<void(bool playing)> onReplayStatusChange;
  };
  static constexpr auto TEXTURE_WIDTH = 560;
  static constexpr auto TEXTURE_HEIGHT = 720;

  SidebarUI(const Sidebar* sidebar, SDL_Renderer* renderer, TTF_Font* font, const Callbacks& callbacks);

  SDL_Texture* render();
  void onClick(int x, int y);

 private:
  SDL_Rect renderText(int x, int y, const std::string& text, const SDL_Color& color);

  const Sidebar* m_sidebar;
  SDL_Renderer* m_renderer;
  TTF_Font* m_font;
  Callbacks m_callbacks;
  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> m_texture;
};

}  // namespace sidebar

#endif  // WSCLIENT_SRC_SIDEBAR_SIDEBAR_UI_H_
