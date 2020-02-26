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

#ifndef UTILS_EXPORT_LOGGER_H_
#define UTILS_EXPORT_LOGGER_H_

#include <cstdint>
#include <string>
#include <unordered_map>

namespace utils
{

class Logger
{
 public:
  // Delete Constructor (static class only)
  Logger() = delete;

  // Each level also include the levels above it
  // ERROR: should always be enabled, to be able to see software errors
  // INFO : can be good to have enabled to see basic information
  // DEBUG: is very verbose and should only be enabled for troubleshooting
  //        specific classes
  enum class Level : std::size_t
  {
    ERROR = 0,
    INFO = 1,
    DEBUG = 2,
  };

  static void log(const char* file_full_path, int line, Level level, ...);
  static void setLevel(const std::string& module, const std::string& level);
  static void setLevel(const std::string& module, Level level);
  static Level getLevel(const std::string& module);

 private:
  static const std::string& levelToString(const Level& level);

  static std::unordered_map<std::string, Level> module_to_level;
};

}  // namespace utils

// Log macros
#define LOG_ERROR(...) utils::Logger::log(__FILE__, __LINE__, utils::Logger::Level::ERROR, __VA_ARGS__)
#define LOG_INFO(...) utils::Logger::log(__FILE__, __LINE__, utils::Logger::Level::INFO, __VA_ARGS__)
#define LOG_DEBUG(...) utils::Logger::log(__FILE__, __LINE__, utils::Logger::Level::DEBUG, __VA_ARGS__)

#endif  // UTILS_EXPORT_LOGGER_H_
