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

#ifndef NETWORK_SRC_CONNECTION_H_
#define NETWORK_SRC_CONNECTION_H_

#include <deque>
#include <memory>
#include <vector>
#include <utility>

#include "incoming_packet.h"
#include "outgoing_packet.h"
#include "logger.h"

/**
 * class Connection
 *
 * Callbacks:
 *   onPacketReceived:   called when a packet has been received
 *
 *   onDisconnected:     called when an error occurs in the receive loop
 *
 *   onConnectionClosed: called when the connection is closed and can safely be deleted
 *
 * There are three ways a connection can be closed:
 *   1. Owner asks to close the connection gracefully, using close(force=false).
 *
 *      Any queued packets will be sent before the connection is closed and the
 *      onConnectionClosed callback is called. If there are no queued packets to
 *      be sent the onConnectionClosed callback is directly.
 *
 *      No more packets are received after the call to close().
 *
 *      NOTE: If an error occurs while sending the queued packets the onConnectionClosed
 *            will never be called. At least not by Connection itself.
 *      TODO(simon): Fix this, somehow.
 *
 *   2. Owner asks to close the connection forcefully, using close(force=true).
 *
 *      The onConnectionClosed callback is called and the owner can delete the connection.
 *      Any queued packets to send will be lost and no more packets will be received.
 *
 *   3. An error occurs in the receive loop.
 *
 *      Both the onDisconnected callback and the onConnectionClosed callback is
 *      called (in that order).
 *
 * Connection handles its receive loop itself, which is started in its constructor:
 *   1. receivePacket()
 *   2. receivePacket lambda
 *   3. onPacketHeaderReceived()
 *   4. onPacketHeaderReceived lambda
 *   5. onPacketDataReceived()
 *
 * Any errors that occur when sending packets are logged but not handled.
 */
template <typename Backend>
class Connection
{
 public:
  struct Callbacks
  {
    std::function<void(IncomingPacket*)> onPacketReceived;
    std::function<void(void)> onDisconnected;
    std::function<void(void)> onConnectionClosed;
  };

  Connection(typename Backend::Socket socket, const Callbacks& callbacks)
    : socket_(std::move(socket)),
      callbacks_(callbacks),
      shutdown_(false)
  {
    // Start to receive packets
    receivePacket();
  }

  virtual ~Connection()
  {
    if (!shutdown_)
    {
      LOG_ERROR("%s: called with shutdown_: false", __func__);
    }
  }

  // Delete copy constructors
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  void close(bool force)
  {
    if (shutdown_)
    {
      LOG_ERROR("%s: called with shutdown_: true", __func__);
      return;
    }

    shutdown_ = true;

    if (force || outgoingPackets_.empty())
    {
      // Either we should force close the connection, or we should not force close but
      // there are no queued packets to send, so close the connection now
      LOG_DEBUG("%s: closing the connection now", __func__);
      closeConnection();
    }
    else
    {
      // If we should not force close it and there are packets left to send, just
      // set shutdown_ to true and the sendPacket handler will call close again
      // when all packets have been sent

      LOG_DEBUG("%s: closing the connection when all queued packets have been sent", __func__);
    }
  }

  void sendPacket(OutgoingPacket&& packet)
  {
    if (shutdown_)
    {
      // The connection is either closing or closed, don't allow more packets to be sent
      LOG_DEBUG("%s: cannot send packet, shutdown_: true", __func__);
      return;
    }

    outgoingPackets_.push_back(std::move(packet));

    // Start to send packet if this is the only packet in the queue
    if (outgoingPackets_.size() == 1)
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

    const auto& packet = outgoingPackets_.front();
    auto packet_length = packet.getLength();

    LOG_DEBUG("%s: sending packet header, data length: %d", __func__, packet_length);

    outgoingHeaderBuffer_[0] = packet_length & 0xFF;
    outgoingHeaderBuffer_[1] = (packet_length >> 8) & 0xFF;

    Backend::async_write(socket_,
                         outgoingHeaderBuffer_.data(),
                         2,
                         [this](const typename Backend::ErrorCode& errorCode, std::size_t len)
                         {
                           if (errorCode)
                           {
                             // Let receive loop handle the disconnect
                             LOG_DEBUG("%s: errorCode: %s", __func__, errorCode.message().c_str());
                             return;
                           }

                           if (len != 2u)
                           {
                             LOG_ERROR("%s: len: %d, expected: 2", __func__, len);
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
                           if (errorCode)
                           {
                             // Let receive loop handle the disconnect
                             LOG_DEBUG("%s: errorCode: %s", __func__, errorCode.message().c_str());
                             return;
                           }

                           if (len != packet_length)
                           {
                             LOG_ERROR("%s: len: %d, expected: d", __func__, len, packet_length);
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
    else if (shutdown_)
    {
      // The outgoing packet queue is now empty, let's close the connection
      closeConnection();
    }
  }

  void receivePacket()
  {
    Backend::async_read(socket_,
                        readBuffer_.data(),
                        2,
                        [this](const typename Backend::ErrorCode& errorCode, std::size_t len)
                        {
                          // Just abort on operation_aborted, this instance might have been deleted
                          if (errorCode == Backend::Error::operation_aborted)
                          {
                            LOG_DEBUG("%s: operation_aborted", __func__);
                            return;
                          }

                          if (errorCode)
                          {
                            LOG_DEBUG("%s: errorCode: %s", __func__, errorCode.message().c_str());
                            callbacks_.onDisconnected();
                            shutdown_ = true;
                            closeConnection();
                            return;
                          }

                          if (len != 2u)
                          {
                            LOG_ERROR("%s: len: %d, expected: 2", __func__, len);
                            callbacks_.onDisconnected();
                            shutdown_ = true;
                            closeConnection();
                            return;
                          }

                          onPacketHeaderReceived();
                        });
  }

  void onPacketHeaderReceived()
  {
    if (shutdown_)
    {
      // Don't continue if we are about to shut down
      return;
    }

    // Receive data
    const auto dataLength = (readBuffer_[1] << 8) | readBuffer_[0];

    LOG_DEBUG("%s: received packet header, data length: %d", __func__, dataLength);

    Backend::async_read(socket_,
                        readBuffer_.data(),
                        dataLength,
                        [this, dataLength](const typename Backend::ErrorCode& errorCode, std::size_t len)
                        {
                          // Just abort on operation_aborted, this instance might have been deleted
                          if (errorCode == Backend::Error::operation_aborted)
                          {
                            LOG_DEBUG("%s: operation_aborted", __func__);
                            return;
                          }

                          if (errorCode)
                          {
                            LOG_DEBUG("%s: errorCode: %s", __func__, errorCode.message().c_str());
                            callbacks_.onDisconnected();
                            shutdown_ = true;
                            closeConnection();
                            return;
                          }

                          if (static_cast<int>(len) != dataLength)
                          {
                            LOG_ERROR("%s: len: %d, expected: %d", __func__, len, dataLength);
                            callbacks_.onDisconnected();
                            shutdown_ = true;
                            closeConnection();
                            return;
                          }

                          onPacketDataReceived(len);
                        });
  }

  void onPacketDataReceived(std::size_t len)
  {
    if (shutdown_)
    {
      // Don't continue if we are about to shut down
      return;
    }

    LOG_DEBUG("%s: received packet data, data length: %d", __func__, len);

    // Call handler
    // Maybe it should stated somewhere that the IncomingPacket is only valid to read/use
    // during the onPacketReceived call
    IncomingPacket packet(readBuffer_.data(), len);
    callbacks_.onPacketReceived(&packet);

    // Receive more packets
    receivePacket();
  }

  void closeConnection()
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

    callbacks_.onConnectionClosed();
  }

  typename Backend::Socket socket_;
  Callbacks callbacks_;

  bool shutdown_;

  // I/O Buffers
  std::array<std::uint8_t, 8192> readBuffer_;

  std::array<std::uint8_t, 2> outgoingHeaderBuffer_;
  std::deque<OutgoingPacket> outgoingPackets_;
};

#endif  // NETWORK_SRC_CONNECTION_H_
