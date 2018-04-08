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

const std::unordered_map<std::string, Logger::Module> Logger::file_to_module_ =
{
  // utils
  { "config_parser.h",      Module::UTILS       },

  // account
  { "account.cc",           Module::ACCOUNT     },

  // network
  { "connection.h",         Module::NETWORK     },
  { "server_impl.h",        Module::NETWORK     },
  { "incoming_packet.cc",   Module::NETWORK     },
  { "outgoing_packet.cc",   Module::NETWORK     },
  { "acceptor.h",           Module::NETWORK     },

  // world
  { "item.cc",              Module::WORLD       },
  { "tile.cc",              Module::WORLD       },
  { "world.cc",             Module::WORLD       },
  { "creature.cc",          Module::WORLD       },
  { "position.cc",          Module::WORLD       },
  { "item_factory.cc",      Module::WORLD       },
  { "world_factory.cc",     Module::WORLD       },

  // loginserver
  { "loginserver.cc",       Module::LOGINSERVER },

  // gameengine
  { "container_manager.cc", Module::GAMEENGINE  },
  { "game_engine.cc",       Module::GAMEENGINE  },
  { "game_engine_queue.cc", Module::GAMEENGINE  },
  { "player.cc",            Module::GAMEENGINE  },

  // worldserver
  { "protocol_71.cc",       Module::WORLDSERVER },
  { "worldserver.cc",       Module::WORLDSERVER },
};

std::unordered_map<Logger::Module, Logger::Level, Logger::ModuleHash> Logger::module_to_level_ =
{
  // Default settings
  { Module::ACCOUNT,     Level::ERROR },
  { Module::GAMEENGINE,  Level::DEBUG },
  { Module::LOGINSERVER, Level::ERROR },
  { Module::NETWORK,     Level::ERROR },
  { Module::UTILS,       Level::ERROR },
  { Module::WORLD,       Level::DEBUG },
  { Module::WORLDSERVER, Level::DEBUG },
};

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

  // Get the Module for this filename
  if (file_to_module_.count(filename) == 0)
  {
    // Not in levels map, always print it
    printf("Logger::log: ERROR: Filename not in Logger::file_to_module_: %s\n", filename);
    return;
  }

  const auto& module = file_to_module_.at(filename);

  // Get the current level for this module
  if (module_to_level_.count(module) == 0)
  {
    printf("Logger::log: ERROR: Module not in Logger::module_to_level_: %d\n", static_cast<int>(module));
    return;
  }

  const auto& moduleLevel = module_to_level_.at(module);

  // Only print if given level is less than or equal to moduleLevel
  // e.g. if INFO is enabled we print ERROR and INFO
  if (level <= moduleLevel)
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

    printf("[%s][%s:%d] %s: %s\n", time_str, filename, line, levelToString(level).c_str(), message);
  }
}

void Logger::setLevel(Module module, const std::string& level)
{
  if (level == "INFO")
  {
    setLevel(module, Logger::Level::INFO);
  }
  else if (level == "DEBUG")
  {
    setLevel(module, Logger::Level::DEBUG);
  }
  else if (level == "ERROR")
  {
    setLevel(module, Logger::Level::ERROR);
  }
  else
  {
    printf("%s: ERROR: Level %s is invalid!\n",
           __func__,
           level.c_str());
  }
}

void Logger::setLevel(Module module, Level level)
{
  if (module_to_level_.count(module) == 0)
  {
    printf("%s: ERROR: Module %d does not exist in module_to_level_!\n",
           __func__,
           static_cast<int>(module));
    return;
  }

  module_to_level_[module] = level;
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
