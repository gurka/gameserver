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

#include "position.h"

#include <sstream>

const Position Position::INVALID = Position();

Position::Position()
  : x_(0),
    y_(0),
    z_(0)
{
}

Position::Position(std::uint16_t x, std::uint16_t y, std::uint8_t z)
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
  ss << "(" << x_ << ", " << y_ << ", " << static_cast<int>(z_) << ")";
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
