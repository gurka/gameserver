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

#include "position.h"

#include <sstream>

const Position Position::INVALID = Position();

Position::Position()
  : x_(0),
    y_(0),
    z_(0)
{
}

Position::Position(uint16_t x, uint16_t y, uint8_t z)
  : x_(x),
    y_(y),
    z_(z)
{
}

bool Position::operator==(const Position& other) const
{
  return (x_ == other.x_) && (y_ == other.y_) && (z_ == other.z_);
}

bool Position::operator!=(const Position& other) const
{
  return !(*this == other);
}

std::string Position::toString() const
{
  std::ostringstream ss;
  ss << "(" << x_ << ", " << y_ << ", " << static_cast<uint16_t>(z_) << ")";
  return ss.str();
}

Position Position::addDirection(const Direction& direction) const
{
  switch (direction)
  {
  case Direction::EAST:
    return Position(x_ + 1, y_    , z_);
  case Direction::NORTH:
    return Position(x_    , y_ - 1, z_);
  case Direction::SOUTH:
    return Position(x_    , y_ + 1, z_);
  case Direction::WEST:
    return Position(x_ - 1, y_    , z_);
  }

  return Position();
}

std::size_t Position::Hash::operator()(const Position& position) const
{
  // The hash is 32 bits, x is cut to 15 bytes, y is cut to 15 bytes and z is cut to 2 bytes:
  // (z is only 0..15)
  // zzyyyyyyyyyyyyyyyxxxxxxxxxxxxxxx
  std::size_t hash = position.x_ & 0x7FFF;
  hash |= ((position.y_ & 0x7FFF) << 15);
  hash |= ((position.z_ & 0x2) << 30);
  return hash;
}
