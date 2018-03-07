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

#include "logger.h"

#include <cstdio>
#include <ctime>
#include <cstdarg>
#include <cstring>

void Logger::log(const char* fileFullPath, int line, Level level, ...)
{
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

  printf("[%s][%s:%d] UNITTEST: %s\n", time_str, filename, line, message);
}

void Logger::setLevel(Module module, const std::string& level)
{
}

void Logger::setLevel(Module module, Level level)
{
}
