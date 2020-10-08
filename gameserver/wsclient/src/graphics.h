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

#ifndef WSCLIENT_SRC_GRAPHICS_H_
#define WSCLIENT_SRC_GRAPHICS_H_

#include <string>

#include "wsworld.h"
#include "position.h"
#include "creature.h"

namespace wsclient::graphics
{

bool init(const utils::data_loader::ItemTypes* itemtypes_in, const std::string& sprite_filename);
void setWindowSize(int width, int height);
void draw(const wsworld::Map& map);
void createCreatureTexture(const wsworld::Creature& creature);
void removeCreatureTexture(const wsworld::Creature& creature);
common::Position screenToMapPosition(int x, int y);

}  // namespace wsclient::graphics

#endif  // WSCLIENT_SRC_GRAPHICS_H_
