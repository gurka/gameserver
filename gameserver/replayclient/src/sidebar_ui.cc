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

#include "sidebar_ui.h"

#include "sidebar.h"
#include "utils/logger.h"

namespace
{

const SDL_Color WHITE  = { 255U, 255U, 255U, 255U };
const SDL_Color BLACK  = {   0U,   0U,   0U, 255U };
const SDL_Color BROWN  = { 102U,  51U,   0U, 255U };
const SDL_Color GRAY   = { 107U, 107U,  71U, 255U };
const SDL_Color YELLOW = { 255U, 204U,   0U, 255U };

const SDL_Rect resume_pause_button_rect = { 12, 12, 48, 24 };

}

namespace sidebar
{

SidebarUI::SidebarUI(const Sidebar* sidebar, SDL_Renderer* renderer, TTF_Font* font, const Callbacks& callbacks)
    : m_sidebar(sidebar),
      m_renderer(renderer),
      m_font(font),
      m_callbacks(callbacks),
      m_texture(nullptr, SDL_DestroyTexture)
{
  m_texture.reset(SDL_CreateTexture(m_renderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    TEXTURE_WIDTH,
                                    TEXTURE_HEIGHT));
}

SDL_Texture* SidebarUI::render()
{
  SDL_SetRenderTarget(m_renderer, m_texture.get());

  // Render border
  SDL_SetRenderDrawColor(m_renderer, BROWN.r, BROWN.g, BROWN.b, BROWN.a);
  SDL_RenderClear(m_renderer);

  SDL_SetRenderDrawColor(m_renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
  const SDL_Rect border_rect = { 6, 6, (TEXTURE_WIDTH / 2) - 6 - 6 - 1, TEXTURE_HEIGHT - 6 - 6 - 1 };
  SDL_RenderFillRect(m_renderer, &border_rect);

  const auto& replay_info = m_sidebar->getReplayInfo();

  // Render resume/pause button
  SDL_SetRenderDrawColor(m_renderer, GRAY.r, GRAY.g, GRAY.b, GRAY.a);
  SDL_RenderFillRect(m_renderer, &resume_pause_button_rect);
  renderText(resume_pause_button_rect.x + 2,
             resume_pause_button_rect.y + 2,
             replay_info.playing ? "Pause" : "Resume",
             WHITE);

  // Render playback info
  renderText(12, 40, "Played packets: " + std::to_string(replay_info.packets_played), WHITE);
  renderText(12, 56, " Total packets: " + std::to_string(replay_info.packets_total), WHITE);

  return m_texture.get();
}

void SidebarUI::onClick(int x, int y)
{
  LOG_INFO(__func__);
  if (resume_pause_button_rect.x <= x &&
      resume_pause_button_rect.x + resume_pause_button_rect.w > x &&
      resume_pause_button_rect.y <= y &&
      resume_pause_button_rect.y + resume_pause_button_rect.h > y)
  {
    m_callbacks.onReplayStatusChange(!m_sidebar->getReplayInfo().playing);
  }
}

SDL_Rect SidebarUI::renderText(int x, int y, const std::string& text, const SDL_Color& color)
{
  auto* text_surface = TTF_RenderText_Blended(m_font, text.c_str(), color);
  auto* text_texture = SDL_CreateTextureFromSurface(m_renderer, text_surface);
  SDL_FreeSurface(text_surface);

  int width;
  int height;
  if (SDL_QueryTexture(text_texture, nullptr, nullptr, &width, &height) != 0)
  {
    LOG_ABORT("%s: could not query text texture: %s", __func__, SDL_GetError());
  }

  const SDL_Rect dest = { x, y, width, height };
  SDL_RenderCopy(m_renderer, text_texture, nullptr, &dest);

  return { x, y, width, height };
}

}
