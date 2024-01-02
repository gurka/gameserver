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

namespace
{

static const SDL_Color WHITE  = { 255U, 255U, 255U, 255U };
static const SDL_Color BLACK  = {   0U,   0U,   0U, 255U };
static const SDL_Color BROWN  = { 102U,  51U,   0U, 255U };
static const SDL_Color GRAY   = { 107U, 107U,  71U, 255U };
static const SDL_Color YELLOW = { 255U, 204U,   0U, 255U };

}

namespace chat
{

ChatUI::ChatUI(const Chat* chat, SDL_Renderer* renderer, TTF_Font* font)
    : m_chat(chat),
      m_renderer(renderer),
      m_font(font),
      m_texture(nullptr, SDL_DestroyTexture),
      m_last_rendered_version(-1),
      m_active_channel("Default")
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

  // Render borders
  SDL_SetRenderDrawColor(m_renderer, BROWN.r, BROWN.g, BROWN.b, BROWN.a);
  SDL_RenderClear(m_renderer);

  SDL_SetRenderDrawColor(m_renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
  const SDL_Rect border_rect = { 6, 24, TEXTURE_WIDTH - 6 - 6 - 1, TEXTURE_HEIGHT - 24 - 6 - 1 };
  SDL_RenderFillRect(m_renderer, &border_rect);

  // Render channels
  m_channel_rects.clear();
  SDL_Rect channel_bounding_box;
  channel_bounding_box = renderText(12, 6, "[Default]", m_active_channel == "Default" ? WHITE : GRAY);
  m_channel_rects.push_back({ "Default", channel_bounding_box });
  for (const auto& pair : m_chat->getChannels())
  {
    channel_bounding_box = renderText(channel_bounding_box.x + channel_bounding_box.w + 6,
                                      6,
                                      std::string("[") + pair.second.name + "]",
                                      m_active_channel == pair.second.name ? WHITE : GRAY);
    m_channel_rects.push_back({ pair.second.name, channel_bounding_box });
  }

  // Render messages
  const auto& messages = [this]()
  {
    if (m_active_channel == "Default")
    {
      return m_chat->getDefaultMessages();
    }
    else
    {
      for (const auto& pair : m_chat->getChannels())
      {
        if (m_active_channel == pair.second.name)
        {
          return pair.second.messages;
        }
      }
      for (const auto& pair : m_chat->getPrivateChannels())
      {
        if (m_active_channel == pair.first)
        {
          return pair.second;
        }
      }
    }

    LOG_ERROR("%s: could not find active channel: %s", __func__, m_active_channel.c_str());
    return m_chat->getDefaultMessages();
  }();
  for (auto i = 0; i < 10; i++)
  {
    if (i >= static_cast<int>(messages.size()))
    {
      break;
    }

    const auto& message = messages[messages.size() - 1 - i];
    const auto text = std::string("[") + std::to_string((std::uint16_t)message.talk_type) + "] " + message.talker + ": " + message.text;
    renderText(12, 192 - 24 - (16 * i), text, YELLOW);
  }

  m_last_rendered_version = m_chat->getVersion();
  return m_texture.get();
}

void ChatUI::onClick(int x, int y)
{
  for (const auto& channel_rect : m_channel_rects)
  {
    if (channel_rect.rect.x <= x &&
        channel_rect.rect.x + channel_rect.rect.w > x &&
        channel_rect.rect.y <= y &&
        channel_rect.rect.y + channel_rect.rect.h > y)
    {
      m_active_channel = channel_rect.channel_name;
      m_last_rendered_version = -1;  // force render
      break;
    }
  }
}

SDL_Rect ChatUI::renderText(int x, int y, const std::string& text, const SDL_Color& color)
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

}  // namespace chat