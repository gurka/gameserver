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

#ifndef WSCLIENT_SRC_CHAT_CHAT_UI_H_
#define WSCLIENT_SRC_CHAT_CHAT_UI_H_

#include <memory>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace chat
{

class Chat;

class ChatUI
{
 public:
  static constexpr auto TEXTURE_WIDTH = 720;
  static constexpr auto TEXTURE_HEIGHT = 192;

  ChatUI(const Chat* chat, SDL_Renderer* renderer);

  SDL_Texture* render();
  void onClick(int x, int y);

 private:
  const Chat* m_chat;
  SDL_Renderer* m_renderer;
  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> m_texture;
  int m_last_rendered_version;

  // TODO: this will break if there are multiple channels with the same name, e.g. a player with name Default
  std::string m_active_channel;

  struct ChannelRect
  {
    std::string channel_name;
    SDL_Rect rect;
  };
  std::vector<ChannelRect> m_channel_rects;
};

}  // namespace chat

#endif  // WSCLIENT_SRC_CHAT_CHAT_UI_H_
