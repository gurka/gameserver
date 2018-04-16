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

#include "outgoing_packet.h"

#include <algorithm>

#include "logger.h"

// Initialize static packet pool
std::stack<std::unique_ptr<std::array<std::uint8_t, 8192>>> OutgoingPacket::buffer_pool_;

OutgoingPacket::OutgoingPacket()
  : position_(0)
{
  if (buffer_pool_.empty())
  {
    buffer_ = std::make_unique<std::array<std::uint8_t, 8192>>();
    LOG_DEBUG("Allocated new buffer");
  }
  else
  {
    buffer_ = std::move(buffer_pool_.top());
    buffer_pool_.pop();
    LOG_DEBUG("Retrieved buffer from pool, buffers now in pool: %lu",
                buffer_pool_.size());
  }
}

OutgoingPacket::~OutgoingPacket()
{
  if (buffer_)
  {
    buffer_pool_.push(std::move(buffer_));
    LOG_DEBUG("Returned buffer to pool, buffers now in pool: %lu",
              buffer_pool_.size());
  }
}

void OutgoingPacket::skipBytes(std::size_t num_bytes)
{
  std::fill(buffer_->begin() + position_, buffer_->begin() + position_ + num_bytes, 0);
  position_ += num_bytes;
}

void OutgoingPacket::addU8(std::uint8_t val)
{
  buffer_->at(position_++) = val;
}

void OutgoingPacket::addU16(std::uint16_t val)
{
  buffer_->at(position_++) = val;
  buffer_->at(position_++) = val >> 8;
}

void OutgoingPacket::addU32(std::uint32_t val)
{
  buffer_->at(position_++) = val;
  buffer_->at(position_++) = val >> 8;
  buffer_->at(position_++) = val >> 16;
  buffer_->at(position_++) = val >> 24;
}

void OutgoingPacket::addString(const std::string& string)
{
  addU16(string.length());
  for (auto c : string)
  {
    buffer_->at(position_++) = c;
  }
}
