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

#include "chat_ui.h"

#include "chat.h"
#include "utils/logger.h"

namespace chat
{

ChatUI::ChatUI(const Chat* chat, SDL_Renderer* renderer, TTF_Font* font)
    : m_chat(chat),
      m_renderer(renderer),
      m_font(font),
      m_texture(nullptr, SDL_DestroyTexture),
      m_last_rendered_version(-1)
{
  m_texture.reset(SDL_CreateTexture(m_renderer,
                                    SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    TEXTURE_WIDTH,
                                    TEXTURE_HEIGHT));
}

SDL_Texture* ChatUI::render()
{
  if (m_last_rendered_version == m_chat->getVersion())
  {
    return m_texture.get();
  }

  SDL_SetRenderTarget(m_renderer, m_texture.get());
  SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(m_renderer);

  const SDL_Color color =
  {
    42U,
    42U,
    42U,
    255U
  };
  const auto& messages = m_chat->getDefaultMessages();
  LOG_INFO("========================================= FOUND %u MESSAGES TO RENDER", messages.size());
  for (auto i = 0; i < 6; i++)
  {
    if (i >= static_cast<int>(messages.size()))
    {
      break;
    }

    const auto& message = messages[messages.size() - 1 - i];
    const auto text = message.talker + ": " + message.text + " (type=" + std::to_string((std::uint16_t)message.talk_type) + ")";
    auto* text_surface = TTF_RenderText_Blended(m_font, text.c_str(), color);
    auto* text_texture = SDL_CreateTextureFromSurface(m_renderer, text_surface);
    SDL_FreeSurface(text_surface);

    int width;
    int height;
    if (SDL_QueryTexture(text_texture, nullptr, nullptr, &width, &height) != 0)
    {
      LOG_ABORT("%s: could not query text texture: %s", __func__, SDL_GetError());
    }

    // i = 0 -> y = 176
    // i = 1 -> y = 160
    const SDL_Rect dest
    {
      16,
      192 - 16 - (16 * i),
      width,
      height
    };
    SDL_RenderCopy(m_renderer, text_texture, nullptr, &dest);
  }

  m_last_rendered_version = m_chat->getVersion();
  return m_texture.get();
}

}  // namespace chat