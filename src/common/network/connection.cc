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

#include "connection.h"

#include <boost/bind.hpp>

#include "incomingpacket.h"
#include "outgoingpacket.h"
#include "logger.h"

namespace BIP = boost::asio::ip;

Connection::Connection(BIP::tcp::socket socket, const Callbacks& callbacks)
  : socket_(std::move(socket)),
    callbacks_(callbacks),
    state_(CONNECTED)
{
  // Start to receive packets
  receivePacket();
}

Connection::~Connection()
{
  if (state_ != CLOSED)
  {
    close(false);
  }
}

void Connection::close(bool gracefully)
{
  if (gracefully && !outgoingPacketBuffers_.empty())
  {
    // If we should close the connection gracefully and there are packets to send
    // just set the state and close the connection when all packets have been sent
    LOG_DEBUG("close() wait until all packets have been sent, setting state to CLOSING");
    state_ = CLOSING;
  }
  else if (state_ != CLOSED)
  {
    // Close the socket
    boost::system::error_code error;

    socket_.shutdown(BIP::tcp::socket::shutdown_both, error);
    if (error)
    {
      LOG_ERROR("%s: Could not shutdown socket: %s", __func__, error.message().c_str());
    }

    socket_.close(error);
    if (error)
    {
      LOG_ERROR("%s: Could not close socket: %s", __func__, error.message().c_str());
    }

    // Set state and call OnCloseHandler
    state_ = CLOSED;
    callbacks_.onConnectionClosed();
  }
}

void Connection::sendPacket(const OutgoingPacket& packet)
{
  outgoingPacketBuffers_.push_back(packet.getBuffer());

  // Start to send packet if this is the only packet in the queue
  if (outgoingPacketBuffers_.size() == 1)
  {
    sendPacketInternal();
  }
}

void Connection::sendPacketInternal()
{
  if (outgoingPacketBuffers_.empty())
  {
    LOG_ERROR("There are no packets to send");
    return;
  }

  auto onPacketDataSent = [this]
                          (const boost::system::error_code& errorCode, std::size_t len)
  {
    if (errorCode)
    {
      LOG_ERROR("Could not send packet");
      close(false);
      return;
    }

    outgoingPacketBuffers_.pop_front();
    if (!outgoingPacketBuffers_.empty())
    {
      // More packet(s) to send
      LOG_DEBUG("Sending next packet in queue, number of packets now in queue: %d",
                  outgoingPacketBuffers_.size());
      sendPacketInternal();
    }
    else if (state_ == CLOSING)
    {
      // We can only have state CLOSING if we want a graceful shutdown
      close(true);
    }
  };

  auto onPacketHeaderSent = [this, onPacketDataSent]
                            (const boost::system::error_code& errorCode, std::size_t len)
  {
    if (errorCode)
    {
      LOG_ERROR("Could not send packet header");
      close(false);
      return;
    }

    LOG_DEBUG("Packet header sent, sending data");
    boost::asio::async_write(socket_,
                             boost::asio::buffer(outgoingPacketBuffers_.front(),
                                                 outgoingPacketBuffers_.front().size()),
                             onPacketDataSent);
  };

  int packet_length = outgoingPacketBuffers_.front().size();
  LOG_DEBUG("Sending packet header, data length: %d", packet_length);
  outgoingHeaderBuffer_[0] = packet_length & 0xFF;
  outgoingHeaderBuffer_[1] = (packet_length >> 8) & 0xFF;
  boost::asio::async_write(socket_,
                           boost::asio::buffer(outgoingHeaderBuffer_, 2),
                           onPacketHeaderSent);
}

void Connection::receivePacket()
{
  auto onPacketDataReceived = [this]
                              (const boost::system::error_code& errorCode, std::size_t len)
  {
    if (errorCode)
    {
      LOG_ERROR("Could not receive packet");
      close(false);
      return;
    }

    LOG_DEBUG("Received packet data");

    // Call handler
    incomingPacket_.resetPosition();
    callbacks_.onPacketReceived(&incomingPacket_);

    if (state_ == CONNECTED)
    {
      // Receive more packets
      receivePacket();
    }
  };

  auto onPacketHeaderReceived = [this, onPacketDataReceived]
                                (const boost::system::error_code& errorCode, std::size_t len)
  {
    if (errorCode)
    {
      LOG_ERROR("Could not receive packet header: %s", errorCode.message().c_str());
      close(false);
      return;
    }

    // Receive data
    incomingPacket_.setLength((incomingHeaderBuffer_[1] << 8) | incomingHeaderBuffer_[0]);
    LOG_DEBUG("Received packet header, data length: %d", incomingPacket_.getLength());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(incomingPacket_.getBuffer(), incomingPacket_.getLength()),
                            onPacketDataReceived);
  };

  boost::asio::async_read(socket_,
                          boost::asio::buffer(incomingHeaderBuffer_, 2),
                          onPacketHeaderReceived);
}
