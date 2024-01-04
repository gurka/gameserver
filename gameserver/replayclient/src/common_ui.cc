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

#include "common_ui.h"

#include <unordered_map>
#include <SDL2/SDL_ttf.h>

#include "bitmap_font.h"
#include "utils/logger.h"

namespace
{

SDL_Renderer* renderer;
std::unique_ptr<BitmapFont> bitmap_font;
std::unordered_map<std::string, TTF_Font*> fonts;

TTF_Font* getFont(int size, bool bold)
{
  const auto key = std::to_string(size) + std::to_string(bold);
  if (fonts.count(key) == 0)
  {
#ifdef EMSCRIPTEN
    TTF_Font* font = TTF_OpenFont("files/DejaVuSansMono.ttf", size);
#else
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", size);
#endif

    if (!font)
    {
      LOG_ABORT("%s: could not open font: %s", __func__, TTF_GetError());
    }
    fonts[key] = font;
  }
  return fonts.at(key);
}

SDL_Texture* getTextTexture(int size, bool bold, const std::string& text, const SDL_Color& color)
{
  // TODO: need to hash size+bold+text+color
  return nullptr;
}

}

namespace ui::common
{

void init(SDL_Renderer* renderer)
{
  ::renderer = renderer;
  bitmap_font = std::make_unique<BitmapFont>(::renderer);
  if (!bitmap_font->load("files/font.txt", "files/font.bmp"))
  {
    LOG_ABORT("%s: could not load BitmapFont", __func__);
  }
}

BitmapFont* get_bitmap_font()
{
  return bitmap_font.get();
}

SDL_Rect renderText(int x, int y, int size, bool bold, const std::string& text, const SDL_Color& color)
{
  auto* font = getFont(size, bold);
  auto* text_surface = TTF_RenderText_Blended(font, text.c_str(), color);
  auto* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
  SDL_FreeSurface(text_surface);

  int width;
  int height;
  if (SDL_QueryTexture(text_texture, nullptr, nullptr, &width, &height) != 0)
  {
    LOG_ABORT("%s: could not query text texture: %s", __func__, SDL_GetError());
  }

  const SDL_Rect dest = { x, y, width, height };
  SDL_RenderCopy(renderer, text_texture, nullptr, &dest);

  return { x, y, width, height };
}

}
