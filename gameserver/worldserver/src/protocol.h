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

#ifndef WORLDSERVER_SRC_PROTOCOL_H_
#define WORLDSERVER_SRC_PROTOCOL_H_

#include <string>

#include "player_ctrl.h"
#include "player.h"
#include "creature.h"
#include "position.h"
#include "item.h"

class IncomingPacket;

class Protocol : public PlayerCtrl
{
 public:
  Protocol() = default;
  virtual ~Protocol() = default;

  // Delete copy constructors
  Protocol(const Protocol&) = delete;
  Protocol& operator=(const Protocol&) = delete;

  // Called by WorldServer
  virtual void disconnected() = 0;

  // Called by Server
  virtual void parsePacket(IncomingPacket* packet) = 0;
};

#endif  // WORLDSERVER_SRC_PROTOCOL_H_
