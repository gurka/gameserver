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

#include "main_ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "game_ui.h"
#include "chat_ui.h"
#include "sidebar_ui.h"
#include "utils/logger.h"

namespace
{


//       game width       sidebar width
//  _______________________________
// |                        |      |
// |                        |      |
// |                  game  |      |
// | 528             height |      |
// |                        |      |  sidebar
// |                        |      |  height
// | +   chat width         |      |
// |________________________|      |
// |                  chat  |      |
// | 192              height|      |
// |________________________|______|
//   =       720            +  560   = 1280
//  720
//
// window is 1280x720
// game is 480x352 but scaled 1.5x -> 720x528
// sidebar is 560x720
// chat is 720x192
//
// game renders at 0,0
// chat renders at 0,528
// sidebar renders at 720,0

SDL_Window* sdl_window = nullptr;
SDL_Renderer* sdl_renderer = nullptr;

game::GameUI* game_ui = nullptr;
chat::ChatUI* chat_ui = nullptr;
sidebar::SidebarUI* sidebar_ui = nullptr;

}

namespace main_ui
{

bool init()
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    LOG_ERROR("%s: could not initialize SDL: %s", __func__, SDL_GetError());
    return false;
  }

  // Create the window initially as maximized
  sdl_window = SDL_CreateWindow("replay client",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                1280,
                                720,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!sdl_window)
  {
    LOG_ERROR("%s: could not create window: %s", __func__, SDL_GetError());
    return false;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
  if (!sdl_renderer)
  {
    LOG_ERROR("%s: could not create renderer: %s", __func__, SDL_GetError());
    return false;
  }

  if (TTF_Init() != 0)
  {
    LOG_ERROR("%s: could not initialize SDL TTF: %s", __func__, TTF_GetError());
    return false;
  }

  return true;
}

SDL_Renderer* get_renderer()
{
  return sdl_renderer;
}

void setGameUI(game::GameUI* game_ui)
{
  ::game_ui = game_ui;
}

void setChatUI(chat::ChatUI* chat_ui)
{
  ::chat_ui = chat_ui;
}

void setSidebarUI(sidebar::SidebarUI* sidebar_ui)
{
  ::sidebar_ui = sidebar_ui;
}

void render()
{
  SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(sdl_renderer);

  // Render game
  auto* game_texture = game_ui->render();
  SDL_SetRenderTarget(sdl_renderer, nullptr);
  const auto scale = 1.5f;
  const SDL_Rect game_dest =
  {
    0,
    0,
    static_cast<int>(480 * scale),
    static_cast<int>(352 * scale),
  };
  SDL_RenderCopy(sdl_renderer, game_texture, nullptr, &game_dest);

  // Render chat
  auto* chat_texture = chat_ui->render();
  SDL_SetRenderTarget(sdl_renderer, nullptr);
  const SDL_Rect chat_dest =
  {
    0,
    528,
    720,
    192
  };
  SDL_RenderCopy(sdl_renderer, chat_texture, nullptr, &chat_dest);

  // Render sidebar
  auto* sidebar_texture = sidebar_ui->render();
  SDL_SetRenderTarget(sdl_renderer, nullptr);
  const SDL_Rect sidebar_dest =
  {
    720,
    0,
    560,
    720
  };
  SDL_RenderCopy(sdl_renderer, sidebar_texture, nullptr, &sidebar_dest);

  SDL_RenderPresent(sdl_renderer);
}

void onClick(int x, int y)
{
  if (x >= 0 && x < 720 && y >= 0 && y < 528)
  {
    game_ui->onClick(x / 1.5f, y / 1.5f);
  }
  else if (x >= 0 && x < 720 && y >= 528 && y < (528 + 192))
  {
    chat_ui->onClick(x, y - 528);
  }
  else if (x >= 720 && x < (720 + 560) && y >= 0 && y < 720)
  {
    sidebar_ui->onClick(x - 720, y);
  }
}

}