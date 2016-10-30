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

#include "logger.h"

const std::unordered_map<std::string, Logger::Module> Logger::file_to_module_ =
{
  // src/utils
  { "configparser.h",     Module::UTILS       },

  // src/account
  { "account.cc",         Module::ACCOUNT     },

  // src/network
  { "connection.cc",      Module::NETWORK     },
  { "server.cc",          Module::NETWORK     },
  { "incomingpacket.cc",  Module::NETWORK     },
  { "outgoingpacket.cc",  Module::NETWORK     },
  { "acceptor.cc",        Module::NETWORK     },

  // src/world
  { "item.cc",            Module::WORLD       },
  { "tile.cc",            Module::WORLD       },
  { "world.cc",           Module::WORLD       },
  { "creature.cc",        Module::WORLD       },
  { "position.cc",        Module::WORLD       },
  { "itemfactory.cc",     Module::WORLD       },
  { "worldfactory.cc",    Module::WORLD       },

  // src/loginserver
  { "loginserver.cc",     Module::LOGINSERVER },

  // src/worldserver
  { "playerctrl.cc",      Module::WORLDSERVER },
  { "gameengine.cc",      Module::WORLDSERVER },
  { "worldserver.cc",     Module::WORLDSERVER },
  { "player.cc",          Module::WORLDSERVER },
};

std::unordered_map<Logger::Module, Logger::Level, Logger::ModuleHash> Logger::module_to_level_ =
{
  // Default settings
  { Module::ACCOUNT,     Level::ERROR },
  { Module::LOGINSERVER, Level::ERROR },
  { Module::NETWORK,     Level::DEBUG },
  { Module::UTILS,       Level::ERROR },
  { Module::WORLD,       Level::DEBUG },
  { Module::WORLDSERVER, Level::DEBUG },
};
