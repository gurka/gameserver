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

#include "connection.h"

#include <utility>

#include <boost/bind.hpp>  //NOLINT

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
    // Force a close
    close(true);
  }
}

void Connection::close(bool force)
{
  if (state_ == CLOSED)
  {
    // Connection already closed
    return;
  }
  else if (force || outgoingPackets_.empty())
  {
    // Either we should force close the connection or we should not force close but
    // there are not any more packets to send, so close the connection now

    LOG_DEBUG("%s: closing the connection now", __func__);

    boost::system::error_code error;

    socket_.shutdown(BIP::tcp::socket::shutdown_both, error);
    if (error)
    {
      LOG_ERROR("%s: could not shutdown socket: %s", __func__, error.message().c_str());
    }

    socket_.close(error);
    if (error)
    {
      LOG_ERROR("%s: could not close socket: %s", __func__, error.message().c_str());
    }

    state_ = CLOSED;
    callbacks_.onConnectionClosed();
  }
  else if (state_ != CLOSING)
  {
    // If we should not force close it and there are packets left to send, just
    // set state to CLOSING and the sendPacket handler will call close again
    // when all packets have been sent

    LOG_DEBUG("%s: closing the connection when all queued packets have been sent", __func__);
    state_ = CLOSING;
  }
}

void Connection::sendPacket(OutgoingPacket&& packet)
{
  if (state_ != CONNECTED)
  {
    // The connection is either closing or close, don't allow more packets to be sent
    LOG_DEBUG("%s: cannot send packet, state = %s",
              __func__,
              state_ == CLOSING ? "CLOSING" : "CLOSED");
    return;
  }

  outgoingPackets_.push_back(std::move(packet));

  // Start to send packet if this is the only packet in the queue
  if (outgoingPackets_.size() == 1)
  {
    sendPacketInternal();
  }
}

void Connection::sendPacketInternal()
{
  if (outgoingPackets_.empty())
  {
    LOG_ERROR("%s: there are no packets to send, __func__");
    return;
  }

  const auto& packet = outgoingPackets_.front();
  auto packet_length = packet.getLength();

  LOG_DEBUG("%s: sending packet header, data length: %d", __func__, packet_length);

  outgoingHeaderBuffer_[0] = packet_length & 0xFF;
  outgoingHeaderBuffer_[1] = (packet_length >> 8) & 0xFF;

  boost::asio::async_write(socket_,
                           boost::asio::buffer(outgoingHeaderBuffer_, 2),
                           std::bind(&Connection::onPacketHeaderSent, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::onPacketHeaderSent(const boost::system::error_code& errorCode, std::size_t len)
{
  if (errorCode == boost::asio::error::operation_aborted)
  {
    // Connection closed, just return
    return;
  }
  else if (errorCode)
  {
    LOG_DEBUG("%s: could not send packet header", __func__);

    callbacks_.onDisconnected();
    close(true);

    return;
  }

  LOG_DEBUG("%s: packet header sent, sending data", __func__);

  const auto& packet = outgoingPackets_.front();
  boost::asio::async_write(socket_,
                           boost::asio::buffer(packet.getBuffer(),
                                               packet.getLength()),
                           std::bind(&Connection::onPacketDataSent, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::onPacketDataSent(const boost::system::error_code& errorCode, std::size_t len)
{
  if (errorCode == boost::asio::error::operation_aborted)
  {
    // Connection closed, just return
    return;
  }
  else if (errorCode)
  {
    LOG_DEBUG("%s: could not send packet", __func__);

    callbacks_.onDisconnected();
    close(true);

    return;
  }

  outgoingPackets_.pop_front();
  if (!outgoingPackets_.empty())
  {
    // More packet(s) to send
    LOG_DEBUG("%s: sending next packet in queue, number of packets in queue: %u",
              __func__,
              outgoingPackets_.size());

    sendPacketInternal();
  }
  else if (state_ == CLOSING)
  {
    // The outgoing packet queue is now empty, let's close the connection
    close(true);  // true or false doesn't really matter
  }
}

void Connection::receivePacket()
{
  boost::asio::async_read(socket_,
                          boost::asio::buffer(incomingHeaderBuffer_, 2),
                          std::bind(&Connection::onPacketHeaderReceived, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::onPacketHeaderReceived(const boost::system::error_code& errorCode, std::size_t len)
{
  if (errorCode == boost::asio::error::operation_aborted)
  {
    // Connection closed, just return
    return;
  }
  else if (errorCode)
  {
    LOG_DEBUG("%s: could not receive packet header: %s",
              __func__, errorCode.message().c_str());

    callbacks_.onDisconnected();
    close(true);

    return;
  }
  else if (state_ != CONNECTED)
  {
    // Don't continue if we are CLOSING or CLOSED
    return;
  }

  // Receive data
  incomingPacket_.setLength((incomingHeaderBuffer_[1] << 8) | incomingHeaderBuffer_[0]);

  LOG_DEBUG("%s: received packet header, data length: %d",
            __func__,
            incomingPacket_.getLength());

  boost::asio::async_read(socket_,
                          boost::asio::buffer(incomingPacket_.getBuffer(), incomingPacket_.getLength()),
                          std::bind(&Connection::onPacketDataReceived, this, std::placeholders::_1, std::placeholders::_2));
}

void Connection::onPacketDataReceived(const boost::system::error_code& errorCode, std::size_t len)
{
  if (errorCode == boost::asio::error::operation_aborted)
  {
    // Connection closed, just return
    return;
  }
  else if (errorCode)
  {
    LOG_DEBUG("%s: could not receive packet", __func__);

    callbacks_.onDisconnected();
    close(true);

    return;
  }
  else if (state_ != CONNECTED)
  {
    // Don't continue if we are CLOSING or CLOSED
    return;
  }

  LOG_DEBUG("%s: received packet data", __func__);

  // Call handler
  incomingPacket_.resetPosition();
  callbacks_.onPacketReceived(&incomingPacket_);

  if (state_ == CONNECTED)
  {
    // Receive more packets
    receivePacket();
  }
}
