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

#include <sstream>

#include "sidebar.h"
#include "common_ui.h"
#include "utils/logger.h"
#include "bitmap_font.h"

namespace
{

const SDL_Rect resume_pause_button_rect = { 12, 12, 48, 24 };

}

namespace sidebar
{

using ui::common::BLACK;
using ui::common::BROWN;
using ui::common::GRAY;
using ui::common::WHITE;

SidebarUI::SidebarUI(const Sidebar* sidebar, SDL_Renderer* renderer, const Callbacks& callbacks)
    : m_sidebar(sidebar),
      m_renderer(renderer),
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
  ui::common::renderText(resume_pause_button_rect.x + 2,
                         resume_pause_button_rect.y + 2,
                         12,
                         false,
                         replay_info.playing ? "Pause" : "Resume",
                         WHITE);

  // Render playback info
  ui::common::renderText(12, 40, 12, false, "Played packets: " + std::to_string(replay_info.packets_played), WHITE);
  ui::common::renderText(12, 56, 12, false, " Total packets: " + std::to_string(replay_info.packets_total), WHITE);

  // Test
  /*
  char c = 32;
  for (int i = 0; i < 6; ++i)
  {
    std::ostringstream oss;
    for (int j = 0; j < 32; ++j)
    {
      oss << (c != 127 ? c : ' ') << ' ';
      c += 1;
    }
    ui::common::get_bitmap_font()->renderText(12, 100 + (i * 20), oss.str());

    if (c == -128)
    {
      c = 160;
    }
  }
   */

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

}
