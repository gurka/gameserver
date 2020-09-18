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

#ifndef UTILS_EXPORT_FILE_READER_H_
#define UTILS_EXPORT_FILE_READER_H_

#include <fstream>
#include <string>

namespace utils
{

class FileReader
{
 public:
  bool load(const std::string& filename)
  {
    m_ifs = std::ifstream(filename, std::ios::binary);
    return static_cast<bool>(m_ifs);
  }

  int offset()
  {
    return m_ifs.tellg();
  }

  void set(int offset)
  {
    m_ifs.seekg(offset);
  }

  void skip(int n)
  {
    m_ifs.seekg(n, std::ifstream::cur);
  }

  std::uint8_t readU8()
  {
    return m_ifs.get();
  }

  std::uint16_t readU16()
  {
    std::uint16_t val = readU8();
    val |= (readU8() << 8);
    return val;
  }

  std::uint32_t readU32()
  {
    std::uint32_t val = readU8();
    val |= (readU8() << 8);
    val |= (readU8() << 16);
    val |= (readU8() << 24);
    return val;
  }

 private:
  std::ifstream m_ifs;
};

}  // namespace utils

#endif  // UTILS_EXPORT_FILE_READER_H_
