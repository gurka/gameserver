/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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

#ifndef WORLD_POSITION_H_
#define WORLD_POSITION_H_

#include <string>
#include "direction.h"

class Position
{
 public:
  static const Position INVALID;

  Position();
  Position(uint16_t x, uint16_t y, uint8_t z);

  bool operator==(const Position& other) const;
  bool operator!=(const Position& other) const;

  std::string toString() const;
  Position addDirection(const Direction& direction) const;

  uint16_t getX() const { return x_; }
  uint16_t getY() const { return y_; }
  uint8_t getZ() const { return z_; }

  struct Hash
  {
    std::size_t operator()(const Position& position) const;
  };

 private:
  uint16_t x_;
  uint16_t y_;
  uint8_t  z_;
};

#endif  // WORLD_POSITION_H_
