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

#include "replay_connection.h"

#include "logger.h"

using boost::posix_time::millisec;
using boost::posix_time::ptime;
using boost::posix_time::microsec_clock;

ReplayConnection::ReplayConnection(asio::io_context* io_context,
                                   std::function<void(void)> on_close,
                                   std::unique_ptr<network::Connection>&& connection)
    : m_timer(*io_context),
      m_timer_started(false),
      m_on_close(std::move(on_close)),
      m_connection(std::move(connection))
{
  // Setup network::Connection
  network::Connection::Callbacks callbacks =
  {
    // onPacketReceived
    [](network::IncomingPacket* packet)
    {
      (void)packet;
      LOG_DEBUG("onPacketReceived -> ignore");
    },

    // onDisconnected
    [this]()
    {
      LOG_DEBUG("onDisconnected");
      if (m_timer_started)
      {
        m_connection.reset();
        m_timer.cancel();
        // calling on_close is done by the timer handler
      }
      else
      {
        m_connection.reset();
        m_on_close();  // instance deleted by this call
      }
    }
  };
  m_connection->init(callbacks, true);

  // Open replay file
  if (!m_replay.load("replays/amazonshield.trp"))
  {
    LOG_ERROR("%s: could not load replay file: %s", __func__, m_replay.getErrorStr().c_str());
    closeConnection();  // instance might be deleted by this call
    return;
  }

  m_replay_start_time = microsec_clock::universal_time();
  sendNextPacket();
}

void ReplayConnection::sendNextPacket()
{
  // Send all packets that it is time for to send
  const auto now = ptime(microsec_clock::universal_time());
  const auto elapsed_ms = (now - m_replay_start_time).total_milliseconds();
  while (m_replay.getNumberOfPacketsLeft() > 0U && m_replay.getNextPacketTime() < elapsed_ms)
  {
    LOG_INFO("%s: sending a packet!", __func__);
    m_connection->sendPacket(m_replay.getNextPacket());
  }

  if (m_replay.getNumberOfPacketsLeft() == 0U)
  {
    LOG_DEBUG("%s: replay done", __func__);
    closeConnection();  // instance might be deleted by this call
    return;
  }

  // Setup timer to expire when it's time to send the next packet
  m_timer.expires_from_now(millisec(m_replay.getNextPacketTime() - elapsed_ms));
  m_timer.async_wait([this](const std::error_code& ec)
  {
    m_timer_started = false;
    if (ec)
    {
      LOG_DEBUG("%s: async_wait error: %s", __func__, ec.message().c_str());
      closeConnection();  // instance might be deleted by this call
      return;
    }
    sendNextPacket();
  });
  m_timer_started = true;
}

void ReplayConnection::closeConnection()
{
  if (m_connection)
  {
    // If we are still connected then close it
    // and we will call on_close in the onDisconnected callback
    m_connection->close(true);
  }
  else
  {
    // Connection already closed, so now delete this instances
    m_on_close();  // instance deleted by this call
  }
}
