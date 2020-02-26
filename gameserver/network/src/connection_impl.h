/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Simon Sandström
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

namespace network
{

/**
 * class ConnectionImpl
 *
 * Callbacks:
 *   on_packet_received:   called when a packet has been received
 *
 *   on_disconnected:     called when the connection is closed and
 *                       this instance is ready for deletion
 *
 * There are three ways a connection can be closed:
 *   1. Owner asks to close the connection gracefully, using close(force=false).
 *
 *      Any queued packets will be sent before the connection is closed.
 *
 *      When all queued packets have been sent and the socket is closed the
 *      on_disconnected callback is called.
 *
 *      If there are no queued packets to send the on_disconnected callback
 *      is called either in the same context or in a later context, depending on
 *      if there are any send or receive calls in progress.
 *
 *      No more packets are received after the call to close().
 *
 *   2. Owner asks to close the connection forcefully, using close(force=true).
 *
 *      The socket is closed and any queued packets to send are lost. The
 *      on_disconnected callback is called, either in the same context or in
 *      a later context, depending on if there are any send or receive calls
 *      in progress.
 *
 *      No more packets are received after the call to close().
 *
 *   3. An error occurs in a send or receive call.
 *
 *      The on_disconnected callback is called as soon as there is no send and
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
    : m_socket(std::move(socket)),
      m_closing(false),
      m_receive_in_progress(false),
      m_send_in_progress(false)
  {
  }

  ~ConnectionImpl() override
  {
    if (m_receive_in_progress || m_send_in_progress)
    {
      LOG_ERROR("%s: called with m_closing: %s, m_receive_in_progress: %s, m_send_in_progress: %s",
                __func__,
                (m_closing           ? "true" : "false"),
                (m_receive_in_progress ? "true" : "false"),
                (m_send_in_progress    ? "true" : "false"));
    }
  }

  // Delete copy constructors
  ConnectionImpl(const ConnectionImpl&) = delete;
  ConnectionImpl& operator=(const ConnectionImpl&) = delete;

  void init(const Callbacks& callbacks) override
  {
    m_callbacks = callbacks;
    receivePacket();
  }

  void close(bool force) override
  {
    if (m_closing)
    {
      LOG_ERROR("%s: called with shutdown_: true", __func__);
      return;
    }

    m_closing = true;

    LOG_DEBUG("%s: force: %s, m_receive_in_progress: %s, m_send_in_progress: %s",
              __func__,
              (force              ? "true" : "false"),
              (m_receive_in_progress ? "true" : "false"),
              (m_send_in_progress    ? "true" : "false"));

    // We can close the socket now if either we should force close,
    // or if there are no send in progress (i.e. no queued packets)
    if (force || !m_send_in_progress)
    {
      closeSocket();  // Note that this instance might be deleted during this call
    }

    // Else: we should send all queued packets before closing the connection
  }

  void sendPacket(OutgoingPacket&& packet) override
  {
    if (m_closing)
    {
      // We are about to close the connection, so don't allow more packets to be sent
      LOG_DEBUG("%s: cannot send packet, m_closing: true", __func__);
      return;
    }

    m_outgoing_packets.push_back(std::move(packet));

    // Start to send packet if this is the only packet in the queue
    if (!m_send_in_progress)
    {
      sendPacketInternal();
    }
  }

 private:
  void sendPacketInternal()
  {
    if (m_outgoing_packets.empty())
    {
      LOG_ERROR("%s: there are no packets to send", __func__);
      return;
    }

    m_send_in_progress = true;

    const auto& packet = m_outgoing_packets.front();
    auto packet_length = packet.getLength();

    LOG_DEBUG("%s: sending packet header, packet length: %d", __func__, packet_length);

    m_outgoing_header_buffer[0] = packet_length & 0xFF;
    m_outgoing_header_buffer[1] = (packet_length >> 8) & 0xFF;

    Backend::async_write(m_socket,
                         m_outgoing_header_buffer.data(),
                         2,
                         [this](const typename Backend::ErrorCode& error_code, std::size_t len)
                         {
                           if (error_code || len != 2u)
                           {
                             LOG_DEBUG("%s: error_code: %s, len: %d (expected: 2)",
                                       __func__,
                                       error_code.message().c_str(),
                                       len);
                             m_send_in_progress = false;
                             closeSocket();  // Note that this instance might be deleted during this call
                             return;
                           }

                           onPacketHeaderSent();
                         });
  }

  void onPacketHeaderSent()
  {
    LOG_DEBUG("%s: packet header sent, sending data", __func__);

    const auto& packet = m_outgoing_packets.front();
    auto packet_length = m_outgoing_packets.front().getLength();
    Backend::async_write(m_socket,
                         packet.getBuffer(),
                         packet.getLength(),
                         [this, packet_length](const typename Backend::ErrorCode& error_code, std::size_t len)
                         {
                           if (error_code || len != packet_length)
                           {
                             LOG_DEBUG("%s: error_code: %s, len: %d (expected: %d)",
                                       __func__,
                                       error_code.message().c_str(),
                                       len,
                                       packet_length);
                             m_send_in_progress = false;
                             closeSocket();  // Note that this instance might be deleted during this call
                             return;
                           }

                           onPacketDataSent();
                         });
  }

  void onPacketDataSent()
  {
    m_outgoing_packets.pop_front();
    if (!m_outgoing_packets.empty())
    {
      // More packet(s) to send
      LOG_DEBUG("%s: sending next packet in queue, number of packets in queue: %u",
                __func__,
                m_outgoing_packets.size());

      sendPacketInternal();
    }
    else
    {
      m_send_in_progress = false;

      if (m_closing)
      {
        closeSocket();  // Note that this instance might be deleted during this call
      }
    }
  }

  void receivePacket()
  {
    m_receive_in_progress = true;

    Backend::async_read(m_socket,
                        m_read_buffer.data(),
                        2,
                        [this](const typename Backend::ErrorCode& error_code, std::size_t len)
                        {
                          if (error_code || len != 2u || m_closing)
                          {
                            LOG_DEBUG("%s: error_code: %s, len: %d (expected: 2), m_closing: %s",
                                      __func__,
                                      error_code.message().c_str(),
                                      len,
                                      (m_closing ? "true" : "false"));
                            m_receive_in_progress = false;
                            closeSocket();  // Note that this instance might be deleted during this call
                            return;
                          }

                          onPacketHeaderReceived();
                        });
  }

  void onPacketHeaderReceived()
  {
    // Receive data
    const auto packet_length = (m_read_buffer[1] << 8) | m_read_buffer[0];

    LOG_DEBUG("%s: received packet header, packet length: %d", __func__, packet_length);

    if (packet_length == 0)
    {
      LOG_DEBUG("%s: packet length 0 is invalid, closing connection", __func__);
      m_receive_in_progress = false;
      closeSocket();
      return;
    }

    Backend::async_read(m_socket,
                        m_read_buffer.data(),
                        packet_length,
                        [this, packet_length](const typename Backend::ErrorCode& error_code, std::size_t len)
                        {
                          if (error_code || static_cast<int>(len) != packet_length || m_closing)
                          {
                            LOG_DEBUG("%s: error_code: %s, len: %d (expected: %d), m_closing: %s",
                                      __func__,
                                      error_code.message().c_str(),
                                      len,
                                      packet_length,
                                      (m_closing ? "true" : "false"));
                            m_receive_in_progress = false;

                            // Only close the socket on error or if m_closing is true and send not in
                            // progress (i.e. close(force=false))
                            if (error_code ||
                                static_cast<int>(len) != packet_length ||
                                (m_closing && !m_send_in_progress))
                            {
                              closeSocket();  // Note that this instance might be deleted during this call
                            }
                            return;
                          }

                          onPacketDataReceived(len);
                        });
  }

  void onPacketDataReceived(std::size_t len)
  {
    LOG_DEBUG("%s: received packet data, packet length: %d", __func__, len);

    // Call handler
    // Maybe it should stated somewhere that the IncomingPacket is only valid to read/use
    // during the on_packet_received call
    IncomingPacket packet(m_read_buffer.data(), len);
    m_callbacks.on_packet_received(&packet);

    // m_closing might have been changed due to the packet that was received above
    // so check it again
    if (m_closing)
    {
      // Don't continue if we are about to shut down
      m_receive_in_progress = false;
      if (!m_send_in_progress)
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
    m_closing = true;

    if (m_socket.is_open())
    {
      typename Backend::ErrorCode error;

      m_socket.shutdown(Backend::shutdown_type::shutdown_both, error);
      if (error)
      {
        LOG_DEBUG("%s: could not shutdown socket: %s", __func__, error.message().c_str());
      }

      m_socket.close(error);
      if (error)
      {
        LOG_DEBUG("%s: could not close socket: %s", __func__, error.message().c_str());
      }
    }

    if (!m_receive_in_progress && !m_send_in_progress)
    {
      // Time to delete this instance
      m_callbacks.on_disconnected();
    }
  }

  typename Backend::Socket m_socket;
  Callbacks m_callbacks;

  bool m_closing;
  bool m_receive_in_progress;
  bool m_send_in_progress;

  // I/O Buffers
  std::array<std::uint8_t, 8192> m_read_buffer;

  std::array<std::uint8_t, 2> m_outgoing_header_buffer;
  std::deque<OutgoingPacket> m_outgoing_packets;
};

}  // namespace network

#endif  // NETWORK_SRC_CONNECTION_IMPL_H_
