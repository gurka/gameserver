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

#ifndef NETWORK_CONNECTION_H_
#define NETWORK_CONNECTION_H_

#include <deque>
#include <memory>
#include <vector>
#include <boost/asio.hpp>  //NOLINT
#include "incomingpacket.h"

// Forward declarations
class OutgoingPacket;

class Connection
{
 public:
  struct Callbacks
  {
    std::function<void(void)> onConnectionClosed;
    std::function<void(IncomingPacket*)> onPacketReceived;
  };

  Connection(boost::asio::ip::tcp::socket socket, const Callbacks& callbacks);
  virtual ~Connection();

  // Delete copy constructors
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  void close(bool gracefully);
  void sendPacket(OutgoingPacket&& packet);

 private:
  void sendPacketInternal();
  void receivePacket();

  boost::asio::ip::tcp::socket socket_;
  Callbacks callbacks_;

  enum State
  {
    CONNECTED,
    CLOSING,
    CLOSED,
  };
  State state_;

  // I/O Buffers
  std::array<uint8_t, 2> incomingHeaderBuffer_;
  IncomingPacket incomingPacket_;

  std::array<uint8_t, 2> outgoingHeaderBuffer_;
  std::deque<OutgoingPacket> outgoingPackets_;
};

#endif  // NETWORK_CONNECTION_H_
