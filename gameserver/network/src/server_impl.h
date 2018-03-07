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

#ifndef NETWORK_SERVERIMPL_H_
#define NETWORK_SERVERIMPL_H_

#include <unordered_map>
#include <utility>

#include "server.h"
#include "acceptor.h"
#include "connection.h"
#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "logger.h"

// Forward declarations
class IncomingPacket;
class OutgoingPacket;

template <typename Backend>
class ServerImpl : public Server
{
 public:
  ServerImpl(typename Backend::Service* io_service,
             unsigned short port,
             const Callbacks& callbacks)
    : acceptor_(io_service,
                port,
                { std::bind(&ServerImpl::onAccept, this, std::placeholders::_1) }),
      callbacks_(callbacks),
      nextConnectionId_(0)
  {
  }

  ~ServerImpl()
  {
    // Close all Connections
    while (!connections_.empty())
    {
      // Connection will call the onConnectionClosed callback
      // which erases it from connections_
      connections_.begin()->second.close(true);
    }
  }

  // Delete copy constructors
  ServerImpl(const ServerImpl&) = delete;
  ServerImpl& operator=(const ServerImpl&) = delete;

  void sendPacket(ConnectionId connectionId, OutgoingPacket&& packet) override
  {
    LOG_DEBUG("%s: connectionId: %d", __func__, connectionId);

    connections_.at(connectionId).sendPacket(std::move(packet));
  }

  void closeConnection(ConnectionId connectionId, bool force) override
  {
    LOG_DEBUG("%s: connectionId: %d", __func__, connectionId);

    connections_.at(connectionId).close(force);
  }

 private:
  // Handler for Acceptor
  void onAccept(typename Backend::Socket socket)
  {
    LOG_DEBUG("%s: connectionId: %d num connections: %lu",
              __func__,
              nextConnectionId_,
              connections_.size() + 1);

    auto connectionId = nextConnectionId_;
    nextConnectionId_ += 1;

    // Create and insert Connection
    typename Connection<Backend>::Callbacks callbacks
    {
      std::bind(&ServerImpl::onPacketReceived, this, connectionId, std::placeholders::_1),
      std::bind(&ServerImpl::onDisconnected, this, connectionId),
      std::bind(&ServerImpl::onConnectionClosed, this, connectionId),
    };
    connections_.emplace(std::piecewise_construct,
                         std::forward_as_tuple(connectionId),
                         std::forward_as_tuple(std::move(socket), callbacks));

    callbacks_.onClientConnected(connectionId);
  }

  // Handler for Connection
  void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet)
  {
    LOG_DEBUG("%s: connectionId: %d", __func__, connectionId);

    callbacks_.onPacketReceived(connectionId, packet);
  }

  void onDisconnected(ConnectionId connectionId)
  {
    LOG_DEBUG("%s: connectionId: %d", __func__, connectionId);

    callbacks_.onClientDisconnected(connectionId);

    // The Connection will call the onConnectionClosed callback after this call
  }

  void onConnectionClosed(ConnectionId connectionId)
  {
    LOG_DEBUG("%s: connectionId: %d num connections: %lu",
              __func__,
              connectionId,
              connections_.size() - 1);

    connections_.erase(connectionId);
  }

  Acceptor<Backend> acceptor_;
  Callbacks callbacks_;

  ConnectionId nextConnectionId_;
  std::unordered_map<ConnectionId, Connection<Backend>> connections_;  // TODO(gurka): vector/array?
};

#endif  // NETWORK_SERVERIMPL_H_
