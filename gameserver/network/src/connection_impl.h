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

#ifndef NETWORK_SRC_CONNECTION_IMPL_H_
#define NETWORK_SRC_CONNECTION_IMPL_H_

#include "connection.h"

#include <deque>
#include <memory>
#include <vector>
#include <utility>

#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "logger.h"

/**
 * class ConnectionImpl
 *
 * Callbacks:
 *   onPacketReceived:   called when a packet has been received
 *
 *   onDisconnected:     called when the connection is closed and
 *                       this instance is ready for deletion
 *
 * There are three ways a connection can be closed:
 *   1. Owner asks to close the connection gracefully, using close(force=false).
 *
 *      Any queued packets will be sent before the connection is closed.
 *
 *      When all queued packets have been sent and the socket is closed the
 *      onDisconnected callback is called.
 *
 *      If there are no queued packets to send the onDisconnected callback
 *      is called either in the same context or in a later context, depending on
 *      if there are any send or receive calls in progress.
 *
 *      No more packets are received after the call to close().
 *
 *   2. Owner asks to close the connection forcefully, using close(force=true).
 *
 *      The socket is closed and any queued packets to send are lost. The
 *      onDisconnected callback is called, either in the same context or in
 *      a later context, depending on if there are any send or receive calls
 *      in progress.
 *
 *      No more packets are received after the call to close().
 *
 *   3. An error occurs in a send or receive call.
 *
 *      The onDisconnected callback is called as soon as there is no send and
 *      no receive call in progress.
 *
 * Connection handles its receive loop itself, which is started in its constructor:
 *   1. receivePacket()
 *   2. receivePacket lambda
 *   3. onPacketHeaderReceived()
 *   4. onPacketHeaderReceived lambda
 *   5. onPacketDataReceived()
 *
 */
template <typename Backend>
class ConnectionImpl : public Connection
{
 public:
  explicit ConnectionImpl(typename Backend::Socket&& socket)
    : socket_(std::move(socket)),
      closing_(false),
      receiveInProgress_(false),
      sendInProgress_(false)
  {
  }

  virtual ~ConnectionImpl()
  {
    if (receiveInProgress_ || sendInProgress_)
    {
      LOG_ERROR("%s: called with closing_: %s, receiveInProgress_: %s, sendInProgress_: %s",
                __func__,
                (closing_           ? "true" : "false"),
                (receiveInProgress_ ? "true" : "false"),
                (sendInProgress_    ? "true" : "false"));
    }
  }

  // Delete copy constructors
  ConnectionImpl(const ConnectionImpl&) = delete;
  ConnectionImpl& operator=(const ConnectionImpl&) = delete;

  void init(const Callbacks& callbacks) override
  {
    callbacks_ = callbacks;
    receivePacket();
  }

  void close(bool force) override
  {
    if (closing_)
    {
      LOG_ERROR("%s: called with shutdown_: true", __func__);
      return;
    }

    closing_ = true;

    LOG_DEBUG("%s: force: %s, receiveInProgress_: %s, sendInProgress_: %s",
              __func__,
              (force              ? "true" : "false"),
              (receiveInProgress_ ? "true" : "false"),
              (sendInProgress_    ? "true" : "false"));

    // We can close the socket now if either we should force close,
    // or if there are no send in progress (i.e. no queued packets)
    if (force || !sendInProgress_)
    {
      closeSocket();  // Note that this instance might be deleted during this call
    }

    // Else: we should send all queued packets before closing the connection
  }

  void sendPacket(OutgoingPacket&& packet) override
  {
    if (closing_)
    {
      // We are about to close the connection, so don't allow more packets to be sent
      LOG_DEBUG("%s: cannot send packet, closing_: true", __func__);
      return;
    }

    outgoingPackets_.push_back(std::move(packet));

    // Start to send packet if this is the only packet in the queue
    if (!sendInProgress_)
    {
      sendPacketInternal();
    }
  }

 private:
  void sendPacketInternal()
  {
    if (outgoingPackets_.empty())
    {
      LOG_ERROR("%s: there are no packets to send", __func__);
      return;
    }

    sendInProgress_ = true;

    const auto& packet = outgoingPackets_.front();
    auto packet_length = packet.getLength();

    LOG_DEBUG("%s: sending packet header, packet length: %d", __func__, packet_length);

    outgoingHeaderBuffer_[0] = packet_length & 0xFF;
    outgoingHeaderBuffer_[1] = (packet_length >> 8) & 0xFF;

    Backend::async_write(socket_,
                         outgoingHeaderBuffer_.data(),
                         2,
                         [this](const typename Backend::ErrorCode& errorCode, std::size_t len)
                         {
                           if (errorCode || len != 2u)
                           {
                             LOG_DEBUG("%s: errorCode: %s, len: %d, expected: 2",
                                       __func__,
                                       errorCode.message().c_str(),
                                       len);
                             closing_ = true;
                             sendInProgress_ = false;
                             closeSocket();  // Note that this instance might be deleted during this call
                             return;
                           }

                           onPacketHeaderSent();
                         });
  }

  void onPacketHeaderSent()
  {
    LOG_DEBUG("%s: packet header sent, sending data", __func__);

    const auto& packet = outgoingPackets_.front();
    auto packet_length = outgoingPackets_.front().getLength();
    Backend::async_write(socket_,
                         packet.getBuffer(),
                         packet.getLength(),
                         [this, packet_length](const typename Backend::ErrorCode& errorCode, std::size_t len)
                         {
                           if (errorCode || len != packet_length)
                           {
                             LOG_DEBUG("%s: errorCode: %s, len: %d, expected: %d",
                                       __func__,
                                       errorCode.message().c_str(),
                                       len,
                                       packet_length);
                             closing_ = true;
                             sendInProgress_ = false;
                             closeSocket();  // Note that this instance might be deleted during this call
                             return;
                           }

                           onPacketDataSent();
                         });
  }

  void onPacketDataSent()
  {
    outgoingPackets_.pop_front();
    if (!outgoingPackets_.empty())
    {
      // More packet(s) to send
      LOG_DEBUG("%s: sending next packet in queue, number of packets in queue: %u",
                __func__,
                outgoingPackets_.size());

      sendPacketInternal();
    }
    else
    {
      sendInProgress_ = false;

      if (closing_)
      {
        closeSocket();  // Note that this instance might be deleted during this call
      }
    }
  }

  void receivePacket()
  {
    receiveInProgress_ = true;

    Backend::async_read(socket_,
                        readBuffer_.data(),
                        2,
                        [this](const typename Backend::ErrorCode& errorCode, std::size_t len)
                        {
                          if (errorCode || len != 2u)
                          {
                            LOG_DEBUG("%s: errorCode: %s, len: %d, expected: 2",
                                      __func__,
                                      errorCode.message().c_str(),
                                      len);
                            closing_ = true;
                            receiveInProgress_ = false;
                            closeSocket();  // Note that this instance might be deleted during this call
                            return;
                          }

                          onPacketHeaderReceived();
                        });
  }

  void onPacketHeaderReceived()
  {
    if (closing_)
    {
      // Don't continue if we are about to shut down
      receiveInProgress_ = false;
      closeSocket();  // Note that this instance might be deleted during this call
      return;
    }

    // Receive data
    const auto packet_length = (readBuffer_[1] << 8) | readBuffer_[0];

    LOG_DEBUG("%s: received packet header, packet length: %d", __func__, packet_length);

    Backend::async_read(socket_,
                        readBuffer_.data(),
                        packet_length,
                        [this, packet_length](const typename Backend::ErrorCode& errorCode, std::size_t len)
                        {
                          if (errorCode || static_cast<int>(len) != packet_length)
                          {
                            LOG_DEBUG("%s: errorCode: %s, len: %d, expected: %d",
                                      __func__,
                                      errorCode.message().c_str(),
                                      len,
                                      packet_length);
                            closing_ = true;
                            receiveInProgress_ = false;
                            closeSocket();  // Note that this instance might be deleted during this call
                            return;
                          }

                          onPacketDataReceived(len);
                        });
  }

  void onPacketDataReceived(std::size_t len)
  {
    if (closing_)
    {
      // Don't continue if we are about to shut down
      receiveInProgress_ = false;
      if (!sendInProgress_)
      {
        closeSocket();  // Note that this instance might be deleted during this call
      }
      return;
    }

    LOG_DEBUG("%s: received packet data, packet length: %d", __func__, len);

    // Call handler
    // Maybe it should stated somewhere that the IncomingPacket is only valid to read/use
    // during the onPacketReceived call
    IncomingPacket packet(readBuffer_.data(), len);
    callbacks_.onPacketReceived(&packet);

    // closing_ might have been changed due to the packet that was received above
    // so check it again
    if (closing_)
    {
      // Don't continue if we are about to shut down
      receiveInProgress_ = false;
      if (!sendInProgress_)
      {
        closeSocket();  // Note that this instance might be deleted during this call
      }
      return;
    }

    // Receive more packets
    receivePacket();
  }

  void closeSocket()
  {
    if (socket_.is_open())
    {
      typename Backend::ErrorCode error;

      socket_.shutdown(Backend::shutdown_type::shutdown_both, error);
      if (error)
      {
        LOG_DEBUG("%s: could not shutdown socket: %s", __func__, error.message().c_str());
      }

      socket_.close(error);
      if (error)
      {
        LOG_DEBUG("%s: could not close socket: %s", __func__, error.message().c_str());
      }
    }

    if (!receiveInProgress_ && !sendInProgress_)
    {
      // Time to delete this instance
      callbacks_.onDisconnected();
    }
  }

  typename Backend::Socket socket_;
  Callbacks callbacks_;

  bool closing_;
  bool receiveInProgress_;
  bool sendInProgress_;

  // I/O Buffers
  std::array<std::uint8_t, 8192> readBuffer_;

  std::array<std::uint8_t, 2> outgoingHeaderBuffer_;
  std::deque<OutgoingPacket> outgoingPackets_;
};

#endif  // NETWORK_SRC_CONNECTION_IMPL_H_
