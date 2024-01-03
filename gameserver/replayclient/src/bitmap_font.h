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

#ifndef WSCLIENT_SRC_BITMAP_FONT_H_
#define WSCLIENT_SRC_BITMAP_FONT_H_

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>

class BitmapFont
{
 public:
  BitmapFont(SDL_Renderer* renderer);
  bool load(const std::string& txt_filename, const std::string& bmp_filename);

  void renderText(int x, int y, const std::string& text, const SDL_Color& color);

 private:
  SDL_Texture* getTexture(const SDL_Color& color);

  SDL_Renderer* m_renderer;

  struct Glyph
  {
    int x;
    int y;
    int width;
  };
  std::array<Glyph, 256> m_glyphs;

  using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
  TexturePtr m_texture;
  int m_texture_width;
  int m_texture_height;

  struct SDLColorHash
  {
    std::size_t operator()(const SDL_Color& color) const;
  };
  struct SDLColorEqualTo
  {
    constexpr bool operator()(const SDL_Color& lhs, const SDL_Color& rhs) const;
  };
  std::unordered_map<SDL_Color, TexturePtr, SDLColorHash, SDLColorEqualTo> m_color_textures;
};

#endif  // WSCLIENT_SRC_BITMAP_FONT_H_
