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

#ifndef NETWORK_SERVERIMPL_H_
#define NETWORK_SERVERIMPL_H_

#include <unordered_map>
#include <boost/asio.hpp>  //NOLINT

#include "server.h"
#include "acceptor.h"
#include "connection.h"

// Forward declarations
class IncomingPacket;
class OutgoingPacket;

using ConnectionId = int;

class ServerImpl : public Server
{
 public:
  ServerImpl(boost::asio::io_service* io_service,
             unsigned short port,
             const Callbacks& callbacks);
  virtual ~ServerImpl();

  bool start();
  void stop();

  void sendPacket(ConnectionId connectionId, OutgoingPacket&& packet);
  void closeConnection(ConnectionId connectionId);

  // Handler for Acceptor
  void onAccept(boost::asio::ip::tcp::socket socket);

  // Handler for Connection
  void onConnectionClosed(ConnectionId connectionId);
  void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet);

 private:
  Acceptor acceptor_;
  Callbacks callbacks_;

  ConnectionId nextConnectionId_;
  std::unordered_map<ConnectionId, Connection> connections_;
};

#endif  // NETWORK_SERVERIMPL_H_
