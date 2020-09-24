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

#include "replay_reader.h"

#include <iterator>

namespace
{

// Internal class for reading values from a file
class FileReader
{
 public:
  bool load(const std::string& filename)
  {
    // Clear m_file_buffer in case a file has been opened previously
    m_file_buffer.clear();

    // Open file
    std::ifstream file_stream;
    file_stream.open(filename, std::ifstream::in | std::ifstream::binary);
    file_stream.unsetf(std::ios::skipws);
    if (!file_stream.good())
    {
      return false;
    }

    // Read file
    m_file_buffer.insert(m_file_buffer.begin(),
                         std::istream_iterator<uint8_t>(file_stream),
                         std::istream_iterator<uint8_t>());
    m_position = 0;

    return true;
  }

  std::size_t getFileLength() const { return m_file_buffer.size(); }

  uint8_t getU8()
  {
    auto value = m_file_buffer[m_position];
    m_position += 1;
    return value;
  }

  uint16_t getU16()
  {
    auto value =  m_file_buffer[m_position] |
                 (m_file_buffer[m_position + 1] << 8);
    m_position += 2;
    return value;
  }

  uint32_t getU32()
  {
    auto value =  m_file_buffer[m_position] |
                 (m_file_buffer[m_position + 1] << 8) |
                 (m_file_buffer[m_position + 2] << 16) |
                 (m_file_buffer[m_position + 3] << 24);
    m_position += 4;
    return value;
  }

  const uint8_t* getBytes(std::size_t length)
  {
    const auto* bytes = &m_file_buffer[m_position];
    m_position += length;
    return bytes;
  }

 private:
  std::vector<uint8_t> m_file_buffer;
  std::size_t m_position;
};

}  // namespace

bool Replay::load(const std::string& filename)
{
  // Clear information about any previosuly opened Replay
  m_version = 0;
  m_length = 0;
  m_packets.clear();
  m_next_packet_index = 0;

  // Load file
  FileReader fr;
  if (!fr.load(filename))
  {
    m_load_error = "Could not open file";
    return false;
  }

  // Read magic number
  const auto magic = fr.getU16();
  if (magic != 0x1337)  // .trp
  {
    m_load_error = "Magic number is not correct";
    return false;
  }

  m_version = fr.getU16();
  m_length = fr.getU32();

  // Read number of replay packets
  const auto num_packets = fr.getU32();

  // Read all replay packets
  for (auto i = 0U; i < num_packets; i++)
  {
    auto packet_time = fr.getU32();
    auto packet_data_length = fr.getU16();
    const auto* packet_data = fr.getBytes(packet_data_length);

    network::OutgoingPacket packet;
    packet.addRawData(packet_data, packet_data_length);
    m_packets.emplace_back(std::move(packet), packet_time);
  }

  return true;
}
