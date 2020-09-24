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

#include "logger.h"

#include <cstdio>
#include <ctime>
#include <cstdarg>
#include <cstring>
#include <array>
#include <sstream>
#include <vector>

namespace
{

std::vector<std::string> getTokens(const std::string& str, char delim)
{
  std::vector<std::string> result;
  std::istringstream iss(str);
  std::string tmp;
  while (std::getline(iss, tmp, delim))
  {
    result.push_back(std::move(tmp));
  }
  return result;
}

}  // namespace

namespace utils
{

std::unordered_map<std::string, Logger::Level> Logger::module_to_level;

void Logger::log(const char* file_full_path, int line, Level level, ...)
{
  // Find module name
  const auto tokens = getTokens(file_full_path, '/');
  if (tokens.size() < 3)
  {
    printf("%s: too few tokens in file_full_path", __func__);
    return;
  }

  const auto& module = tokens[tokens.size() - 3];
  const auto& filename = tokens[tokens.size() - 1];
  const auto module_level = getLevel(module);

  // Only print if given level is less than or equal to module_level
  // e.g. if INFO is enabled we print ERROR and INFO
  if (level <= module_level)
  {
    // Get current date and time
    time_t now = time(nullptr);
    struct tm tstruct{};
    std::array<char, 32> time_str;
    localtime_r(&now, &tstruct);
    strftime(time_str.data(), time_str.size(), "%Y-%m-%d %X", &tstruct);

    // Extract variadic function arguments
    va_list args;
    va_start(args, level);  // Start to extract after the "level"-argument
                            // Which also must be a non-reference according to CppCheck
                            // otherwise va_start invokes undefined behaviour
    const char* format = va_arg(args, const char*);
    std::array<char, 256> message;
    // Use the rest of the arguments together with the
    // format string to construct the actual log message
    vsnprintf(message.data(), message.size(), format, args);
    va_end(args);

    // Assume that the output is a terminal which supports color, for now
    if (level == Level::ERROR)
    {
      printf("\033[31m");
    }
    printf("[%s][%s:%d] %s: %s\n", time_str.data(), filename.c_str(), line, levelToString(level).c_str(), message.data());
    if (level == Level::ERROR)
    {
      printf("\033[00m");
    }

    std::fflush(stdout);
  }
}

void Logger::setLevel(const std::string& module, const std::string& level)
{
  if (level == "INFO")
  {
    setLevel(module, Level::INFO);
  }
  else if (level == "DEBUG")
  {
    setLevel(module, Level::DEBUG);
  }
  else if (level == "ERROR")
  {
    setLevel(module, Level::ERROR);
  }
  else
  {
    printf("%s: ERROR: Level %s is invalid!\n",
           __func__,
           level.c_str());
  }
}

void Logger::setLevel(const std::string& module, Level level)
{
  module_to_level[module] = level;
}

Logger::Level Logger::getLevel(const std::string& module)
{
  if (module_to_level.count(module) == 0)
  {
    // Set default level
    module_to_level[module] = Level::DEBUG;
  }
  return module_to_level[module];
}

const std::string& Logger::levelToString(const Level& level)
{
  static const std::string error("ERROR");
  static const std::string info("INFO");
  static const std::string debug("DEBUG");
  static const std::string invalid("INVALID_LOG_LEVEL");

  switch (level)
  {
    case Level::ERROR:
      return error;
    case Level::INFO:
      return info;
    case Level::DEBUG:
      return debug;
    default:
      return invalid;
  }
}

}  // namespace utils
