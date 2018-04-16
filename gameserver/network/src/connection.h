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
      state_(CONNECTED)
  {
    // Start to receive packets
    receivePacket();
  }

  virtual ~Connection()
  {
    if (state_ != CLOSED)
    {
      // Force a close
      close(true);
    }
  }

  // Delete copy constructors
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  void close(bool force)
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

      typename Backend::ErrorCode error;

      socket_.shutdown(Backend::shutdown_type::shutdown_both, error);
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

  void sendPacket(OutgoingPacket&& packet)
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

 private:
  void sendPacketInternal()
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

    Backend::async_write(socket_,
                         outgoingHeaderBuffer_.data(),
                         2,
                         [this](const typename Backend::ErrorCode& errorCode, std::size_t len)
                         {
                           // Just abort on operation_aborted, this instance might have been deleted
                           if (errorCode == Backend::Error::operation_aborted)
                           {
                             LOG_DEBUG("%s: operation_aborted", __func__);
                             return;
                           }

                           onPacketHeaderSent(errorCode, len);
                         });
  }

  void onPacketHeaderSent(const typename Backend::ErrorCode& errorCode, std::size_t len)
  {
    if (errorCode || len != 2u)
    {
      if (errorCode)
      {
        LOG_DEBUG("%s: could not send packet header: %s",
                  __func__,
                  errorCode.message().c_str());
      }
      else  // if (len != 2u)
      {
        LOG_DEBUG("%s: could not send packet header: bytes sent: %d, expected: 2",
                  __func__,
                  len);
      }

      if (state_ != CLOSED)
      {
        callbacks_.onDisconnected();
        close(true);
      }

      return;
    }

    LOG_DEBUG("%s: packet header sent, sending data", __func__);

    const auto& packet = outgoingPackets_.front();
    Backend::async_write(socket_,
                         packet.getBuffer(),
                         packet.getLength(),
                         [this](const typename Backend::ErrorCode& errorCode, std::size_t len)
                         {
                           // Just abort on operation_aborted, this instance might have been deleted
                           if (errorCode == Backend::Error::operation_aborted)
                           {
                             LOG_DEBUG("%s: operation_aborted", __func__);
                             return;
                           }

                           onPacketDataSent(errorCode, len);
                         });
  }

  void onPacketDataSent(const typename Backend::ErrorCode& errorCode, std::size_t len)
  {
    if (errorCode || len != outgoingPackets_.front().getLength())
    {
      if (errorCode)
      {
        LOG_DEBUG("%s: could not send packet: %s",
                  __func__,
                  errorCode.message().c_str());
      }
      else  // if (len != outgoingPackets_.front().getLength())
      {
        LOG_DEBUG("%s: could not send packet, bytes sent: %d, expected: %d",
                  __func__,
                  len,
                  outgoingPackets_.front().getLength());
      }

      if (state_ != CLOSED)
      {
        callbacks_.onDisconnected();
        close(true);
      }

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

                          onPacketHeaderReceived(errorCode, len);
                        });
  }

  void onPacketHeaderReceived(const typename Backend::ErrorCode& errorCode, std::size_t len)
  {
    if (errorCode || len != 2u)
    {
      if (errorCode)
      {
        LOG_DEBUG("%s: could not receive packet header: %s",
                  __func__,
                  errorCode.message().c_str());
      }
      else  // if (len != 2u)
      {
        LOG_DEBUG("%s: could not receive packet header: bytes received: %d, expected: 2",
                  __func__,
                  len);
      }

      if (state_ != CLOSED)
      {
        callbacks_.onDisconnected();
        close(true);
      }

      return;
    }

    if (state_ != CONNECTED)
    {
      // Don't continue if we are CLOSING or CLOSED
      return;
    }

    // Receive data
    const auto dataLength = (readBuffer_[1] << 8) | readBuffer_[0];

    LOG_DEBUG("%s: received packet header, data length: %d", __func__, dataLength);

    Backend::async_read(socket_,
                        readBuffer_.data(),
                        dataLength,
                        [this](const typename Backend::ErrorCode& errorCode, std::size_t len)
                        {
                          // Just abort on operation_aborted, this instance might have been deleted
                          if (errorCode == Backend::Error::operation_aborted)
                          {
                            LOG_DEBUG("%s: operation_aborted", __func__);
                            return;
                          }

                          onPacketDataReceived(errorCode, len);
                        });
  }

  void onPacketDataReceived(const typename Backend::ErrorCode& errorCode, std::size_t len)
  {
    if (errorCode)
    {
      LOG_DEBUG("%s: could not receive packet", __func__);

      if (state_ != CLOSED)
      {
        callbacks_.onDisconnected();
        close(true);
      }

      return;
    }
    else if (state_ != CONNECTED)
    {
      // Don't continue if we are CLOSING or CLOSED
      return;
    }

    LOG_DEBUG("%s: received packet data, data length: %d", __func__, len);

    // Call handler
    // Maybe it should state somewhere that the IncomingPacket is only valid to read/use
    // during the onPacketReceived call
    IncomingPacket packet(readBuffer_.data(), len);
    callbacks_.onPacketReceived(&packet);

    if (state_ == CONNECTED)
    {
      // Receive more packets
      receivePacket();
    }
  }

  typename Backend::Socket socket_;
  Callbacks callbacks_;

  enum State
  {
    CONNECTED,
    CLOSING,
    CLOSED,
  };
  State state_;

  // I/O Buffers
  std::array<std::uint8_t, 8192> readBuffer_;

  std::array<std::uint8_t, 2> outgoingHeaderBuffer_;
  std::deque<OutgoingPacket> outgoingPackets_;
};

#endif  // NETWORK_SRC_CONNECTION_H_
