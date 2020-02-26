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

#ifndef NETWORK_EXPORT_OUTGOING_PACKET_H_
#define NETWORK_EXPORT_OUTGOING_PACKET_H_

#include <cstdint>
#include <array>
#include <string>
#include <stack>
#include <memory>
#include <vector>

namespace network
{

class OutgoingPacket
{
 public:
  OutgoingPacket();
  virtual ~OutgoingPacket();

  OutgoingPacket(OutgoingPacket&&) = default;
  OutgoingPacket& operator=(OutgoingPacket&&) = default;

  // Delete copy constructors
  OutgoingPacket(const OutgoingPacket&) = delete;
  OutgoingPacket& operator=(const OutgoingPacket&) = delete;

  const std::uint8_t* getBuffer() const { return m_buffer->data(); }
  std::size_t getLength() const { return m_position; }

  void skipBytes(std::size_t num_bytes);
  void addU8(std::uint8_t val);
  void addU16(std::uint16_t val);
  void addU32(std::uint32_t val);
  void addString(const std::string& string);

  // Generic functions
  void add(std::uint8_t val)       { addU8(val);     }
  void add(std::uint16_t val)      { addU16(val);    }
  void add(std::uint32_t val)      { addU32(val);    }
  void add(const std::string& str) { addString(str); }
  void add(const char* str)        { addString(str); }

  // Invalid function to make sure that no implicit conversions / promotion occurs
  template<typename T>
  void add(T) = delete;

 private:
  std::unique_ptr<std::array<std::uint8_t, 8192>> m_buffer;
  std::size_t m_position{0};

  static std::stack<std::unique_ptr<std::array<std::uint8_t, 8192>>> m_bufferpool;
};

}  // namespace network

#endif  // NETWORK_EXPORT_OUTGOING_PACKET_H_
