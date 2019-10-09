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
#ifndef WSCLIENT_SRC_TEXTURE_H_
#define WSCLIENT_SRC_TEXTURE_H_

#include <memory>
#include <vector>

#include <SDL.h>

#include "item.h"
#include "sprite_loader.h"

namespace wsclient
{

class Texture
{
 public:
  static Texture create(SDL_Renderer* renderer,
                        const io::SpriteLoader& sprite_loader,
                        const common::ItemType& item_type);

  common::ItemTypeId getItemTypeId() const { return m_item_type_id; }
  const std::vector<SDL_Texture*>& getTextures() const { return m_texture_ptrs; }

 private:
  common::ItemTypeId m_item_type_id;

  // One for memory management, one for returning
  using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
  std::vector<TexturePtr> m_textures;
  std::vector<SDL_Texture*> m_texture_ptrs;
};

}  // namespace wsclient

#endif  // WSCLIENT_SRC_TEXTURE_H_

