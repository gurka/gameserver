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

#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include <cstdio>
#include <ctime>
#include <cstdarg>
#include <cstring>
#include <string>
#include <unordered_map>

class Logger
{
 public:
  // Each level also include the levels above it
  // ERROR: should always be enabled, to be able to see software errors
  // INFO : can be good to have enabled to see basic information
  // DEBUG: is very verbose and should only be enabled for troubleshooting
  //        specific classes
  enum class Level
  {
    LEVEL_ERROR,
    LEVEL_INFO,
    LEVEL_DEBUG,  // LEVEL_ to avoid conflict with DEBUG-macro
  };

  static void log(const char* fileFullPath, int line, Level level, ...)
  {
    // Get current date and time
    time_t now = time(0);
    struct tm tstruct{};
    char time_str[32];
    localtime_r(&now, &tstruct);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %X", &tstruct);

    // Extract variadic function arguments
    va_list args;
    va_start(args, level);  // Start to extract after the "level"-argument
                            // Which also must be a non-reference according to CppCheck
                            // otherwise va_start invokes undefined behaviour
    const char* format = va_arg(args, const char*);
    char message[256];
    // Use the rest of the arguments together with the
    // format string to construct the actual log message
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // Remove directories in fileFullPath ("network/server.cc" => "server.cc")
    const char* filename;
    if (strrchr(fileFullPath, '/'))
    {
      filename = strrchr(fileFullPath, '/') + 1;
    }
    else
    {
      filename = fileFullPath;
    }

    // Check if the log level is enabled for this file
    if (levels_.count(filename) == 0)
    {
      // Not in levels map, always print it
      printf("[%s][%s:%d] (Filename not in Logger::levels_) %s: %s\n",
             time_str, filename, line, levelToString(level).c_str(), message);
    }
    else
    {
      const auto& currentLevel = levels_.at(filename);

      // Only print if given level is less than or equal to currentLevel
      // If INFO is enabled we print ERROR and INFO, and so on
      if (level <= currentLevel)
      {
        printf("[%s][%s:%d] %s: %s\n", time_str, filename, line, levelToString(level).c_str(), message);
      }
    }
  }

 private:
  static const std::string& levelToString(const Level& level)
  {
    static const std::string error("ERROR");
    static const std::string info("INFO");
    static const std::string debug("DEBUG");
    static const std::string invalid("INVALID_LOG_LEVEL");

    switch (level)
    {
      case Level::LEVEL_ERROR:
        return error;
      case Level::LEVEL_INFO:
        return info;
      case Level::LEVEL_DEBUG:
        return debug;
      default:
        return invalid;
    }
  }

  // Maps a filename to enabled log level
  // TODO(gurka): Be able to change this at runtime or in config file
  static const std::unordered_map<std::string, Level> levels_;
};

// Log macros
#define LOG_ERROR(...) Logger::log(__FILE__, __LINE__, Logger::Level::LEVEL_ERROR, __VA_ARGS__)
#define LOG_INFO(...) Logger::log(__FILE__, __LINE__, Logger::Level::LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::log(__FILE__, __LINE__, Logger::Level::LEVEL_DEBUG, __VA_ARGS__)

#endif  // UTILS_LOGGER_H_
