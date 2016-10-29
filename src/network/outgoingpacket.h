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

#ifndef NETWORK_OUTGOINGPACKET_H_
#define NETWORK_OUTGOINGPACKET_H_

#include <cstdint>
#include <string>
#include <stack>
#include <memory>
#include <vector>

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

  const uint8_t* getBuffer() const { return buffer_->data(); }
  std::size_t getLength() const { return position_; }

  void skipBytes(std::size_t num_bytes);
  void addU8(uint8_t val);
  void addU16(uint16_t val);
  void addU32(uint32_t val);
  void addString(const std::string& string);

 private:
  std::unique_ptr<std::array<uint8_t, 8192>> buffer_;
  std::size_t position_;

  static std::stack<std::unique_ptr<std::array<uint8_t, 8192>>> buffer_pool_;
};

#endif  // NETWORK_OUTGOINGPACKET_H_
