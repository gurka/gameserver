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

#ifndef COMMON_NETWORK_INCOMINGPACKET_H_
#define COMMON_NETWORK_INCOMINGPACKET_H_

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <vector>

class IncomingPacket
{
 public:
  IncomingPacket();
  virtual ~IncomingPacket() = default;

  // Delete copy constructors
  IncomingPacket(const IncomingPacket&) = delete;
  IncomingPacket& operator=(const IncomingPacket&) = delete;

  // Should only be used by Connection
  std::size_t getLength() const { return length_; }
  void setLength(std::size_t length) { length_ = length; }
  std::array<uint8_t, 8192>::pointer getBuffer() { return buffer_.data(); }
  void resetPosition() { position_ = 0; }

  bool isEmpty() const { return position_ >= length_; }
  std::size_t bytesLeft() const { return length_ - position_; }
  uint8_t peekU8() const;
  uint8_t getU8();
  uint16_t peekU16() const;
  uint16_t getU16();
  uint32_t peekU32() const;
  uint32_t getU32();
  std::string getString();
  std::vector<uint8_t> getBytes(int numBytes);

 private:
  std::array<uint8_t, 8192> buffer_;
  std::size_t length_;
  std::size_t position_;
};

#endif  // COMMON_NETWORK_INCOMINGPACKET_H_
