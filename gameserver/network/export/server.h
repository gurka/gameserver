/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
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

#ifndef NETWORK_SERVER_H_
#define NETWORK_SERVER_H_

#include <functional>

class IncomingPacket;
class OutgoingPacket;

using ConnectionId = int;

class Server
{
 public:
  struct Callbacks
  {
    std::function<void(ConnectionId)> onClientConnected;
    std::function<void(ConnectionId)> onClientDisconnected;
    std::function<void(ConnectionId, IncomingPacket*)> onPacketReceived;
  };

  Server() = default;
  virtual ~Server() = default;

  // Delete copy constructors
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  virtual void sendPacket(ConnectionId connectionId, OutgoingPacket&& packet) = 0;
  virtual void closeConnection(ConnectionId connectionId, bool force) = 0;
};

#endif  // NETWORK_SERVER_H_
