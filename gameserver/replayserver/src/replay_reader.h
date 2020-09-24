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

#ifndef REPLAYSERVER_SRC_REPLAY_READER_H_
#define REPLAYSERVER_SRC_REPLAY_READER_H_

#include <cstdint>
#include <fstream>
#include <vector>

#include "incoming_packet.h"
#include "outgoing_packet.h"

class ReplayPacket
{
 public:
  ReplayPacket(network::OutgoingPacket&& packet, uint32_t packet_time)
    : m_packet(std::move(packet)),
      m_packet_time(packet_time)
  {
  }

  network::OutgoingPacket&& getPacket() { return std::move(m_packet); }
  std::uint32_t getPacketTime() const { return m_packet_time; }

 private:
  network::OutgoingPacket m_packet;
  std::uint32_t m_packet_time;
};

class Replay
{
 public:
  // File loading functions
  bool load(const std::string& filename);
  const std::string& getErrorStr() const { return m_load_error; }

  // Replay functions
  uint16_t getVersion() const { return m_version; }
  uint32_t getLength() const { return m_length; }

  std::size_t getNumberOfPackets() const { return m_packets.size(); }
  std::size_t getNumberOfPacketsLeft() const { return m_packets.size() - m_next_packet_index; }

  std::uint32_t getNextPacketTime() const { return m_packets[m_next_packet_index].getPacketTime(); }
  network::OutgoingPacket&& getNextPacket() { return std::move(m_packets[m_next_packet_index++].getPacket()); }

 private:
  std::string m_load_error;

  uint16_t m_version;
  uint32_t m_length;
  std::vector<ReplayPacket> m_packets;
  std::size_t m_next_packet_index;
};

#endif  // REPLAYSERVER_SRC_REPLAY_READER_H_
