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

#include "outgoing_packet.h"

#include <algorithm>

#include "logger.h"

namespace network
{

// Initialize static packet pool
std::stack<std::unique_ptr<std::array<std::uint8_t, 8192>>> OutgoingPacket::m_bufferpool;

OutgoingPacket::OutgoingPacket()
{
  if (m_bufferpool.empty())
  {
    m_buffer = std::make_unique<std::array<std::uint8_t, 8192>>();
    LOG_DEBUG("Allocated new buffer");
  }
  else
  {
    m_buffer = std::move(m_bufferpool.top());
    m_bufferpool.pop();
    LOG_DEBUG("Retrieved buffer from pool, buffers now in pool: %lu",
              m_bufferpool.size());
  }
}

OutgoingPacket::~OutgoingPacket()
{
  if (m_buffer)
  {
    m_bufferpool.push(std::move(m_buffer));
    LOG_DEBUG("Returned buffer to pool, buffers now in pool: %lu",
              m_bufferpool.size());
  }
}

void OutgoingPacket::skipBytes(std::size_t num_bytes)
{
  std::fill(m_buffer->begin() + m_position, m_buffer->begin() + m_position + num_bytes, 0);
  m_position += num_bytes;
}

void OutgoingPacket::addU8(std::uint8_t val)
{
  m_buffer->at(m_position++) = val;
}

void OutgoingPacket::addU16(std::uint16_t val)
{
  m_buffer->at(m_position++) = val;
  m_buffer->at(m_position++) = val >> 8;
}

void OutgoingPacket::addU32(std::uint32_t val)
{
  m_buffer->at(m_position++) = val;
  m_buffer->at(m_position++) = val >> 8;
  m_buffer->at(m_position++) = val >> 16;
  m_buffer->at(m_position++) = val >> 24;
}

void OutgoingPacket::addString(const std::string& string)
{
  addU16(string.length());
  for (auto c : string)
  {
    m_buffer->at(m_position++) = c;
  }
}

}  // namespace network
