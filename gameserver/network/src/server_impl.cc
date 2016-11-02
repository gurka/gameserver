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

#include "server_impl.h"

#include "connection.h"
#include "incomingpacket.h"
#include "outgoingpacket.h"
#include "logger.h"

namespace BIP = boost::asio::ip;

ServerImpl::ServerImpl(boost::asio::io_service* io_service,
               unsigned short port,
               const Callbacks& callbacks)
  : acceptor_(io_service,
              port,
              {
                std::bind(&ServerImpl::onAccept, this, std::placeholders::_1)
              }),
    callbacks_(callbacks),
    nextConnectionId_(0)

{
  LOG_INFO("Starting ServerImpl.");
}

ServerImpl::~ServerImpl()
{
  LOG_INFO("Closing ServerImpl.");
  if (acceptor_.isListening())
  {
    stop();
  }
}

bool ServerImpl::start()
{
  return acceptor_.start();
}

void ServerImpl::stop()
{
  acceptor_.stop();

  // Closing a Connection will erase it from connections_ due to the onConnectionClosed callback
  while (!connections_.empty())
  {
    connections_.begin()->second.close(false);
  }
}

void ServerImpl::sendPacket(ConnectionId connectionId, OutgoingPacket&& packet)
{
  LOG_DEBUG("sendPacket() connectionId: %d", connectionId);
  connections_.at(connectionId).sendPacket(std::move(packet));
}

void ServerImpl::closeConnection(ConnectionId connectionId)
{
  LOG_DEBUG("closeConnection() connectionId: %d", connectionId);
  connections_.at(connectionId).close(true);
}

// Handler for Acceptor
void ServerImpl::onAccept(BIP::tcp::socket socket)
{
  int connectionId = nextConnectionId_++;

  // Create and insert Connection
  Connection::Callbacks callbacks
  {
    std::bind(&ServerImpl::onConnectionClosed, this, connectionId),
    std::bind(&ServerImpl::onPacketReceived, this, connectionId, std::placeholders::_1)
  };
  connections_.emplace(std::piecewise_construct,
                       std::forward_as_tuple(connectionId),
                       std::forward_as_tuple(std::move(socket), callbacks));

  LOG_DEBUG("onServerImplAccept() new connectionId: %d no connections: %lu",
            connectionId,
            connections_.size());

  callbacks_.onClientConnected(connectionId);
}

// Handler for Connection
void ServerImpl::onConnectionClosed(ConnectionId connectionId)
{
  connections_.erase(connectionId);
  LOG_DEBUG("onConnectionClosed() connectionId: %d no connections: %lu",
            connectionId,
            connections_.size());
  callbacks_.onClientDisconnected(connectionId);
}

void ServerImpl::onPacketReceived(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("onPacketReceived() connectionId: %d", connectionId);
  callbacks_.onPacketReceived(connectionId, packet);
}
