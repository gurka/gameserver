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

#ifndef WORLDSERVER_PROTOCOLPOSITION_H_
#define WORLDSERVER_PROTOCOLPOSITION_H_

#include <cstdint>
#include <cstdio>
#include <string>

// A 40 byte object that can be interpreted as one of:
//   - Position in the world
//   - Inventory slot
//   - Container id and slot
// TODO(simon): ItemPosition ?
class ProtocolPosition
{
 public:
  ProtocolPosition(const Position& position)
    : x_(position.getX()),
      y_(position.getY()),
      z_(position.getZ())
  {
  }

  ProtocolPosition(int inventorySlot)
    : x_(0xFFFF),
      y_(inventorySlot),
      z_(0)
  {
  }

  ProtocolPosition(int containerId, int containerSlot)
    : x_(0xFFFF),
      y_(containerId | 0x40),
      z_(containerSlot)
  {
  }

  ProtocolPosition(std::uint16_t x, std::uint16_t y, std::uint8_t z)
    : x_(x),
      y_(y),
      z_(z)
  {
  }

  std::string toString() const
  {
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "(0x%04X 0x%04X 0x%02X)", x_, y_, z_);
    return std::string(buffer);
  }

  // Positions have x value not fully set
  bool isPosition() const { return x_ != 0xFFFF; }
  Position getPosition() const { return Position(x_, y_, z_); }

  // Inventory slots have fully set x value and 7th bit in y value not set
  // Inventory slot is (4 lower bits in) y value
  // (What is z value?)
  bool isInventorySlot() const { return x_ == 0xFFFF && (y_ & 0x40) == 0x00; }
  int getInventorySlot() const { return y_ & ~0x40; }

  // Containers have fully set x value and 7th bit in y value set
  // Container id is lower 6 bits in y value
  // Container slot is z value
  // (Container id is max 6 bits: 0..63?)
  bool isContainer() const { return x_ == 0xFFFF && (y_ & 0x40) == 0x40; }
  int getContainerId() const { return y_ & ~0x40; }
  int getContainerSlot() const { return z_; }

 private:
  std::uint16_t x_;
  std::uint16_t y_;
  std::uint8_t z_;
};

#endif  // WORLDSERVER_PROTOCOLPOSITION_H_
