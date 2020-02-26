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

#include "incoming_packet.h"

#include <algorithm>
#include <vector>

namespace network
{

IncomingPacket::IncomingPacket(const std::uint8_t* buffer, std::size_t length)
  : m_buffer(buffer),
    m_length(length),
    m_position(0)
{
}

std::uint8_t IncomingPacket::peekU8() const
{
  return m_buffer[m_position];
}

std::uint8_t IncomingPacket::getU8()
{
  auto value = peekU8();
  m_position += 1;
  return value;
}

std::uint16_t IncomingPacket::peekU16() const
{
  std::uint16_t value = m_buffer[m_position];
  value |= (static_cast<std::uint16_t>(m_buffer[m_position + 1]) << 8) & 0xFF00;
  return value;
}

std::uint16_t IncomingPacket::getU16()
{
  auto value = peekU16();
  m_position += 2;
  return value;
}

std::uint32_t IncomingPacket::peekU32() const
{
  std::uint32_t value = m_buffer[m_position];
  value |= (static_cast<std::uint32_t>(m_buffer[m_position + 1]) << 8)  & 0xFF00;
  value |= (static_cast<std::uint32_t>(m_buffer[m_position + 2]) << 16) & 0xFF0000;
  value |= (static_cast<std::uint32_t>(m_buffer[m_position + 3]) << 24) & 0xFF000000;
  return value;
}

std::uint32_t IncomingPacket::getU32()
{
  auto value = peekU32();
  m_position += 4;
  return value;
}

std::string IncomingPacket::getString()
{
  std::uint16_t length = getU16();
  int temp = m_position;
  m_position += length;
  return std::string(&m_buffer[temp], &m_buffer[temp + length]);
}

std::vector<std::uint8_t> IncomingPacket::peekBytes(int num_bytes) const
{
  std::vector<std::uint8_t> bytes(&m_buffer[m_position], &m_buffer[m_position + num_bytes]);
  return bytes;
}

std::vector<std::uint8_t> IncomingPacket::getBytes(int num_bytes)
{
  auto bytes = peekBytes(num_bytes);
  m_position += num_bytes;
  return bytes;
}

}  // namespace network
