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

#include "server.h"

#include "connection.h"
#include "incomingpacket.h"
#include "outgoingpacket.h"
#include "logger.h"

namespace BIP = boost::asio::ip;

Server::Server(boost::asio::io_service* io_service,
               unsigned short port,
               const Callbacks& callbacks)
  : acceptor_(io_service,
              port,
              {
                std::bind(&Server::onAccept, this, std::placeholders::_1)
              }),
    callbacks_(callbacks),
    nextConnectionId_(0)

{
  LOG_INFO("Starting Server.");
}

Server::~Server()
{
  LOG_INFO("Closing Server.");
  if (acceptor_.isListening())
  {
    stop();
  }
}

bool Server::start()
{
  return acceptor_.start();
}

void Server::stop()
{
  acceptor_.stop();

  // Closing a Connection will erase it from connections_ due to the onConnectionClosed callback
  // Hence this ugly hack
  while (connections_.begin() != connections_.end())
  {
    connections_.begin()->second->close(false);
  }
}

void Server::sendPacket(ConnectionId connectionId, const OutgoingPacket& packet)
{
  LOG_DEBUG("sendPacket() connectionId: %d", connectionId);
  connections_.at(connectionId)->sendPacket(packet);
}

void Server::closeConnection(ConnectionId connectionId)
{
  LOG_DEBUG("closeConnection() connectionId: %d", connectionId);
  connections_.at(connectionId)->close(true);
}

// Handler for Acceptor
void Server::onAccept(BIP::tcp::socket socket)
{
  int connectionId = nextConnectionId_++;

  // Create and insert Connection
  Connection::Callbacks callbacks
  {
    std::bind(&Server::onConnectionClosed, this, connectionId),
    std::bind(&Server::onPacketReceived, this, connectionId, std::placeholders::_1)
  };
  auto connection = std::unique_ptr<Connection>(new Connection(std::move(socket), callbacks));
  connections_.insert(std::make_pair(connectionId, std::move(connection)));

  LOG_DEBUG("onServerAccept() new connectionId: %d no connections: %lu",
              connectionId, connections_.size());

  callbacks_.onClientConnected(connectionId);
}

// Handler for Connection
void Server::onConnectionClosed(ConnectionId connectionId)
{
  connections_.erase(connectionId);
  LOG_DEBUG("onConnectionClosed() connectionId: %d no connections: %lu",
              connectionId, connections_.size());
  callbacks_.onClientDisconnected(connectionId);
}

void Server::onPacketReceived(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("onPacketReceived() connectionId: %d", connectionId);
  callbacks_.onPacketReceived(connectionId, packet);
}
