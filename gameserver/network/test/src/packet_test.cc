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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "incoming_packet.h"
#include "outgoing_packet.h"

namespace network
{

class PacketTest : public ::testing::Test
{
 public:
 protected:
};

TEST_F(PacketTest, IncomingPacket)
{
  const std::uint8_t packetBuffer[] =
  {
    // 1 byte value 0x11
    0x11,

    // 2 byte value 0x3322
    0x22, 0x33,

    // 4 byte value 0x77665544
    0x44, 0x55, 0x66, 0x77,

    // string length 4 + string "data"
    0x04, 0x00,
    0x64, 0x61, 0x74, 0x61,

    // 3 bytes
    0x12, 0x34, 0x56
  };

  const std::size_t packetBufferLength = 16;

  std::size_t bytesLeft = packetBufferLength;

  // Under test
  IncomingPacket packet(packetBuffer, packetBufferLength);

  EXPECT_FALSE(packet.isEmpty());
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Peek uint8_t
  EXPECT_EQ(static_cast<std::uint8_t>(0x11), packet.peekU8());
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Get uint8_t
  EXPECT_EQ(static_cast<std::uint8_t>(0x11), packet.getU8());
  bytesLeft -= 1;
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Peek uint16_t
  EXPECT_EQ(static_cast<std::uint16_t>(0x3322), packet.peekU16());
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Get uint16_t
  EXPECT_EQ(static_cast<std::uint16_t>(0x3322), packet.getU16());
  bytesLeft -= 2;
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Peek uint32_t
  EXPECT_EQ(static_cast<std::uint32_t>(0x77665544), packet.peekU32());
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Get uint32_t
  EXPECT_EQ(static_cast<std::uint32_t>(0x77665544), packet.getU32());
  bytesLeft -= 4;
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Get string
  EXPECT_EQ("data", packet.getString());
  bytesLeft -= 2 + 4;  // string length + string
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  // Get bytes
  auto bytes = packet.getBytes(3);
  bytesLeft -= 3;
  EXPECT_EQ(0x12, bytes[0]);
  EXPECT_EQ(0x34, bytes[1]);
  EXPECT_EQ(0x56, bytes[2]);
  EXPECT_EQ(bytesLeft, packet.bytesLeft());

  EXPECT_TRUE(packet.isEmpty());
}

TEST_F(PacketTest, OutgoingPacket)
{
  // Under test
  OutgoingPacket packet;

  EXPECT_EQ(0u, packet.getLength());

  // Add values
  packet.addU8(0x11);
  packet.addU16(0x3322);
  packet.addU32(0x77665544);
  packet.addString("data");
  packet.skipBytes(2);
  packet.addU8(0x55);

  // 1 + 2 + 4 + 2 + 4 + 2 + 1 = 16
  EXPECT_EQ(16u, packet.getLength());

  const std::uint8_t* packetBuffer = packet.getBuffer();

  // 0x11
  EXPECT_EQ(0x11, packetBuffer[0]);

  // 0x3322
  EXPECT_EQ(0x22, packetBuffer[1]);
  EXPECT_EQ(0x33, packetBuffer[2]);

  // 0x77665544
  EXPECT_EQ(0x44, packetBuffer[3]);
  EXPECT_EQ(0x55, packetBuffer[4]);
  EXPECT_EQ(0x66, packetBuffer[5]);
  EXPECT_EQ(0x77, packetBuffer[6]);

  // string length (4)
  EXPECT_EQ(0x04, packetBuffer[7]);
  EXPECT_EQ(0x00, packetBuffer[8]);

  // "data"
  EXPECT_EQ(0x64, packetBuffer[9]);
  EXPECT_EQ(0x61, packetBuffer[10]);
  EXPECT_EQ(0x74, packetBuffer[11]);
  EXPECT_EQ(0x61, packetBuffer[12]);

  // 2 bytes skipped

  // 0x55
  EXPECT_EQ(0x55, packetBuffer[15]);
}

}  // namespace network
