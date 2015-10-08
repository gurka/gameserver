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

const std::unordered_map<std::string, Logger::Level> Logger::levels_ =
{
  // src/utils/
  { "configparser.h",     Level::LEVEL_INFO  },

  // src/common/accountmanager
  { "accountmgr.cc",      Level::LEVEL_DEBUG },

  // src/common/network
  { "connection.cc",      Level::LEVEL_DEBUG },
  { "server.cc",          Level::LEVEL_DEBUG },
  { "incomingpacket.cc",  Level::LEVEL_DEBUG },
  { "outgoingpacket.cc",  Level::LEVEL_DEBUG },
  { "acceptor.cc",        Level::LEVEL_DEBUG },

  // src/common/world
  { "item.cc",            Level::LEVEL_DEBUG },
  { "tile.cc",            Level::LEVEL_DEBUG },
  { "world.cc",           Level::LEVEL_DEBUG },
  { "creature.cc",        Level::LEVEL_DEBUG },
  { "position.cc",        Level::LEVEL_DEBUG },

  // src/loginserver
  { "loginserver.cc",     Level::LEVEL_DEBUG },

  // src/worldserver
  { "playerctrl.cc",      Level::LEVEL_DEBUG },
  { "gameengine.cc",      Level::LEVEL_DEBUG },
  { "worldserver.cc",     Level::LEVEL_DEBUG },
  { "player.cc",          Level::LEVEL_DEBUG },
};
