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

#include "position.h"

#include <sstream>

#include "gtest/gtest.h"

namespace common
{

TEST(PositionTest, Constructor)
{
  auto position = Position(0, 0, 0);
  ASSERT_EQ(position.getX(), 0);
  ASSERT_EQ(position.getY(), 0);
  ASSERT_EQ(position.getZ(), 0);

  position = Position(1, 2, 3);
  ASSERT_EQ(position.getX(), 1);
  ASSERT_EQ(position.getY(), 2);
  ASSERT_EQ(position.getZ(), 3);
}

TEST(PositionTest, Equals)
{
  Position first(0, 0, 0);
  Position second(0, 0, 0);
  Position third(1, 2, 3);
  Position fourth(1, 2, 3);

  ASSERT_EQ(first, second);
  ASSERT_EQ(second, first);

  ASSERT_NE(first, third);
  ASSERT_NE(third, first);

  ASSERT_NE(first, fourth);
  ASSERT_NE(fourth, first);

  ASSERT_NE(second, third);
  ASSERT_NE(third, second);

  ASSERT_NE(second, fourth);
  ASSERT_NE(fourth, second);

  ASSERT_EQ(third, fourth);
  ASSERT_EQ(fourth, third);
}

TEST(PositionTest, AddDirection)
{
  Position position(5, 5, 5);

  position = position.addDirection(Direction::NORTH);  // y = y - 1
  ASSERT_EQ(position, Position(5, 4, 5));

  position = position.addDirection(Direction::WEST);   // x = x - 1
  ASSERT_EQ(position, Position(4, 4, 5));

  position = position.addDirection(Direction::SOUTH);  // y = y + 1
  ASSERT_EQ(position, Position(4, 5, 5));

  position = position.addDirection(Direction::EAST);   // x = x + 1
  ASSERT_EQ(position, Position(5, 5, 5));

  position = position.addDirection(Direction::NORTH);  // y = y - 5
  position = position.addDirection(Direction::NORTH);
  position = position.addDirection(Direction::NORTH);
  position = position.addDirection(Direction::NORTH);
  position = position.addDirection(Direction::NORTH);
  ASSERT_EQ(position, Position(5, 0, 5));
}

}  // namespace common
