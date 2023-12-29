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

#ifndef WSCLIENT_SRC_TEXTURE_H_
#define WSCLIENT_SRC_TEXTURE_H_

#include <memory>
#include <vector>

#ifdef EMSCRIPTEN
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "common/creature.h"
#include "common/item.h"
#include "common/direction.h"

#include "sprite_loader.h"

namespace game
{

class Texture
{
 public:
  static Texture createOutfitTexture(SDL_Renderer* renderer,
                                     const SpriteLoader& sprite_loader,
                                     const common::ItemType& item_type,
                                     const common::Outfit& outfit);

  static Texture createItemTexture(SDL_Renderer* renderer,
                                   const SpriteLoader& sprite_loader,
                                   const common::ItemType& item_type);

  common::ItemTypeId getItemTypeId() const { return m_item_type.id; }

  int getNumVersions()   const { return m_item_type.sprite_info.getNumVersions(); }
  int getNumAnimations() const { return m_item_type.sprite_info.getNumAnimations(); }
  int getNumTextures()   const { return m_item_type.sprite_info.getNumTextures(); }

  // Items
  SDL_Texture* getItemTexture(int version, int anim_tick) const;

  // Creature
  SDL_Texture* getCreatureStillTexture(common::Direction direction) const;
  SDL_Texture* getCreatureWalkTexture(common::Direction direction, int walk_tick) const;

  // Missile
  // TODO(simon): add NW, NE, SW and SE to Direction first

 private:
  common::ItemType m_item_type;

  using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
  std::vector<TexturePtr> m_textures;
};

}  // namespace game

#endif  // WSCLIENT_SRC_TEXTURE_H_
