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

#ifndef COMMON_EXPORT_POSITION_H_
#define COMMON_EXPORT_POSITION_H_

#include <cstdint>
#include <string>
#include <sstream>

#include "direction.h"

namespace common
{

class Position
{
 public:
  Position(std::uint16_t x, std::uint16_t y, std::uint8_t z)
      : m_x(x),
        m_y(y),
        m_z(z)
  {
  }

  bool operator==(const Position& other) const
  {
    return (m_x == other.m_x) && (m_y == other.m_y) && (m_z == other.m_z);
  }

  bool operator!=(const Position& other) const
  {
    return !(*this == other);
  }

  std::string toString() const
  {
    std::ostringstream ss;
    ss << "(" << m_x << ", " << m_y << ", " << static_cast<int>(m_z) << ")";
    return ss.str();
  }

  Position addDirection(const Direction& direction) const
  {
    switch (direction)
    {
    case Direction::EAST:
      return { std::uint16_t(m_x + 1),                    m_y, m_z };
    case Direction::NORTH:
      return {                    m_x, std::uint16_t(m_y - 1), m_z };
    case Direction::SOUTH:
      return {                    m_x, std::uint16_t(m_y + 1), m_z };
    case Direction::WEST:
      return { std::uint16_t(m_x - 1),                    m_y, m_z };
    }
    return *this;
  }

  std::uint16_t getX() const { return m_x; }
  std::uint16_t getY() const { return m_y; }
  std::uint8_t  getZ() const { return m_z; }

 private:
  std::uint16_t m_x{0};
  std::uint16_t m_y{0};
  std::uint8_t m_z{0};
};

}  // namespace common

#endif  // COMMON_EXPORT_POSITION_H_
