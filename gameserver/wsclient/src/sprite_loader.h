/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Simon Sandström
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
#ifndef WSCLIENT_SRC_SPRITE_LOADER_H_
#define WSCLIENT_SRC_SPRITE_LOADER_H_

#include <fstream>
#include <string>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;

namespace wsclient::sprite
{

class Reader
{
 public:
  bool load(const std::string& filename);
  SDL_Texture* get_sprite(int sprite_id, SDL_Renderer* renderer);

 private:
  std::uint8_t readU8();
  std::uint16_t readU16();
  std::uint32_t readU32();

  std::ifstream m_ifs;
  std::vector<std::uint32_t> m_offsets;

  struct TextureCache
  {
    int sprite_id;
    SDL_Texture* texture;
  };
  std::vector<TextureCache> m_textures;
};

}  // namespace wsclient::sprite

#endif  // WSCLIENT_SRC_SPRITE_LOADER_H_
