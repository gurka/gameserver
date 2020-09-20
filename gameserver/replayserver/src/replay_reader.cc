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
    // Clear fileBuffer_ in case a file has been opened previously
    fileBuffer_.clear();

    // Open file
    std::ifstream fileStream;
    fileStream.open(filename, std::ifstream::in | std::ifstream::binary);
    fileStream.unsetf(std::ios::skipws);
    if (!fileStream.good())
    {
      return false;
    }

    // Read file
    fileBuffer_.insert(fileBuffer_.begin(),
                       std::istream_iterator<uint8_t>(fileStream),
                       std::istream_iterator<uint8_t>());
    position_ = 0;

    return true;
  }

  std::size_t getFileLength() const { return fileBuffer_.size(); }

  uint8_t getU8()
  {
    auto value = fileBuffer_[position_];
    position_ += 1;
    return value;
  }

  uint16_t getU16()
  {
    auto value =  fileBuffer_[position_] |
                 (fileBuffer_[position_ + 1] << 8);
    position_ += 2;
    return value;
  }

  uint32_t getU32()
  {
    auto value =  fileBuffer_[position_] |
                 (fileBuffer_[position_ + 1] << 8) |
                 (fileBuffer_[position_ + 2] << 16) |
                 (fileBuffer_[position_ + 3] << 24);
    position_ += 4;
    return value;
  }

  const uint8_t* getBytes(std::size_t length)
  {
    const auto* bytes = &fileBuffer_[position_];
    position_ += length;
    return bytes;
  }

 private:
  std::vector<uint8_t> fileBuffer_;
  std::size_t position_;
};

}

bool Replay::load(const std::string& filename)
{
  // Clear information about any previosuly opened Replay
  m_version = 0;
  m_length = 0;
  m_packets.clear();
  m_next_packet_index = 0;

  FileReader fr;

  // Load file
  if (!fr.load(filename))
  {
    m_load_error = "Could not open file";
    return false;
  }

  // Read magic number
  auto magic = fr.getU16();
  if (magic != 0x1337)  // .trp
  {
    m_load_error = "Magic number is not correct";
    return false;
  }

  m_version = fr.getU16();
  m_length = fr.getU32();

  // Read number of replay packets
  auto numberOfPackets = fr.getU32();

  // Read all replay packets
  for (auto i = 0u; i < numberOfPackets; i++)
  {
    auto packetTime = fr.getU32();
    auto packetDataLength = fr.getU16();
    const auto* packetData = fr.getBytes(packetDataLength);

    network::OutgoingPacket packet;
    packet.addRawData(packetData, packetDataLength);
    m_packets.emplace_back(std::move(packet), packetTime);
  }

  return true;
}
