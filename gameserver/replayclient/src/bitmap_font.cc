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

#include "bitmap_font.h"

#include <fstream>
#include <functional>
#include <string>

#include <SDL2/SDL.h>

#include "utils/logger.h"

namespace std
{



}

BitmapFont::BitmapFont(SDL_Renderer* renderer)
    : m_renderer(renderer),
      m_glyphs(),
      m_texture(nullptr, &SDL_DestroyTexture),
      m_texture_width(0),
      m_texture_height(0),
      m_color_textures()
{
}

bool BitmapFont::load(const std::string& txt_filename, const std::string& bmp_filename)
{
  // Open file
  std::ifstream ifs;
  ifs.open(txt_filename);
  if (!ifs.is_open())
  {
    return false;
  }

  // Parse file
  std::string line;
  int character = 32;
  int x = 0;
  int y = 0;
  int width;
  while (std::getline(ifs, line))
  {
    if (line[0] == ';')
    {
      continue;
    }

    if (sscanf(line.c_str(), "%d", &width) != 1)
    {
      LOG_ABORT("%s: could not parse line: \"%s\" in file %s", __func__, line.c_str(), txt_filename.c_str());
    }

    m_glyphs[character] = { x, y, width };
    //LOG_INFO("%s: character=%d x=%d y=%d width=%d", __func__, character, x, y, width);

    x += width + 2;

    ++character;
    if (character == 127)
    {
      character = 160;
    }

    if (character == 64 || character == 96 || character == 160 || character == 192 || character == 224)
    {
      x = 0;
      y += 13;
    }
  }

  // Load bmp
  SDL_Surface* image_surface = SDL_LoadBMP(bmp_filename.c_str());
  if (!image_surface)
  {
    LOG_ERROR("%s: could not load %s", __func__, bmp_filename.c_str());
    return false;
  }
  m_texture.reset(SDL_CreateTextureFromSurface(m_renderer, image_surface));
  if (!m_texture)
  {
    LOG_ERROR("%s: could not load %s", __func__, bmp_filename.c_str());
    return false;
  }
  SDL_FreeSurface(image_surface);

  SDL_QueryTexture(m_texture.get(), nullptr, nullptr, &m_texture_width, &m_texture_height);

  return true;
}

void BitmapFont::renderText(int x, int y, const std::string& text, const SDL_Color& color)
{
  // Note: assumes SDL_SetRenderTarget has been set

  auto* texture = getTexture(color);

  int current_x = x;
  for (auto c : text)
  {
    const auto unsigned_c = static_cast<unsigned char>(c);
    const auto& glyph = m_glyphs[unsigned_c];
    if (glyph.width == 0)
    {
      LOG_ERROR("%s: no glyph for character='%c' (%d)", __func__, unsigned_c, static_cast<int>(unsigned_c));
      // TODO: render a glyph to represent invalid character
      continue;
    }

    // Render this glyph
    const SDL_Rect src_rect = { glyph.x, glyph.y, glyph.width, 12 };
    const SDL_Rect dest_rect = { current_x, y, glyph.width, 12 };
    SDL_RenderCopy(m_renderer, texture, &src_rect, &dest_rect);

    // Note: -1 to merge the outline of this glyph with the outline of the next glyph
    current_x += glyph.width - 1;
  }
}

SDL_Texture* BitmapFont::getTexture(const SDL_Color& color)
{
  if (m_color_textures.count(color) == 0)
  {
    // Save old render target (since it should be set when calling renderText())
    auto* old_render_target = SDL_GetRenderTarget(m_renderer);

    // Create texture
    m_color_textures.emplace(color,
                             TexturePtr(SDL_CreateTexture(m_renderer,
                                                          SDL_PIXELFORMAT_RGBA8888,
                                                          SDL_TEXTUREACCESS_TARGET,
                                                          m_texture_width,
                                                          m_texture_height),
                                        SDL_DestroyTexture));
    auto* texture = m_color_textures.at(color).get();
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    // Render from base to new texture, with color mod
    SDL_SetRenderTarget(m_renderer, texture);
    SDL_SetTextureColorMod(m_texture.get(), color.r, color.g, color.b);
    SDL_RenderCopy(m_renderer, m_texture.get(), nullptr, nullptr);

    // Reset
    SDL_SetRenderTarget(m_renderer, old_render_target);

    LOG_INFO("%s: created new bitmap texture, number of textures are now: %d",
             __func__,
             static_cast<int>(m_color_textures.size()));
  }
  return m_color_textures.at(color).get();
}

std::size_t BitmapFont::SDLColorHash::operator()(const SDL_Color& color) const
{
  auto hash_func = std::hash<decltype(color.r)>();
  std::size_t hash = 17;
  hash = hash * 31 + hash_func(color.r);
  hash = hash * 31 + hash_func(color.g);
  hash = hash * 31 + hash_func(color.b);
  hash = hash * 31 + hash_func(color.a);
  return hash;
}

constexpr bool BitmapFont::SDLColorEqualTo::operator()(const SDL_Color& lhs, const SDL_Color& rhs) const
{
  return lhs.r == rhs.r &&
         lhs.g == rhs.g &&
         lhs.b == rhs.b &&
         lhs.a == rhs.a;
}
