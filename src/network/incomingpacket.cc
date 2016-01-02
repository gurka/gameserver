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

#include "incomingpacket.h"

#include <algorithm>
#include <vector>

IncomingPacket::IncomingPacket()
  : buffer_(),
    length_(0),
    position_(0)
{
}

uint8_t IncomingPacket::peekU8() const
{
  return buffer_[position_];
}

uint8_t IncomingPacket::getU8()
{
  auto value = peekU8();
  position_ += 1;
  return value;
}

uint16_t IncomingPacket::peekU16() const
{
  uint16_t value = buffer_[position_];
  value |= ((uint16_t)buffer_[position_ + 1] << 8) & 0xFF00;
  return value;
}

uint16_t IncomingPacket::getU16()
{
  auto value = peekU16();
  position_ += 2;
  return value;
}

uint32_t IncomingPacket::peekU32() const
{
  uint32_t value = buffer_[position_];
  value |= ((uint32_t)buffer_[position_ + 1] << 8) & 0xFF00;
  value |= ((uint32_t)buffer_[position_ + 2] << 16) & 0xFF0000;
  value |= ((uint32_t)buffer_[position_ + 3] << 24) & 0xFF000000;
  return value;
}

uint32_t IncomingPacket::getU32()
{
  auto value = peekU32();
  position_ += 4;
  return value;
}

std::string IncomingPacket::getString()
{
  uint16_t length = getU16();
  int temp = position_;
  position_ += length;
  return std::string(buffer_.cbegin() + temp,
                     buffer_.cbegin() + temp + length);
}

std::vector<uint8_t> IncomingPacket::getBytes(int num_bytes)
{
  std::vector<uint8_t> bytes(num_bytes);
  std::copy(buffer_.cbegin() + position_,
            buffer_.cbegin() + position_ + num_bytes,
            bytes.begin());
  position_ += num_bytes;
  return bytes;
}
