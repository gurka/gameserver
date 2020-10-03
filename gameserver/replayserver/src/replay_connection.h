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

#ifndef REPLAYSERVER_SRC_REPLAY_CONNECTION_H_
#define REPLAYSERVER_SRC_REPLAY_CONNECTION_H_

#include <functional>
#include <memory>

#include <asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "connection.h"
#include "replay_reader.h"

class ReplayConnection
{
 public:
  ReplayConnection(asio::io_context* io_context,
                   std::function<void(void)> on_close,
                   std::unique_ptr<network::Connection>&& connection);

 private:
  void sendNextPacket();
  void closeConnection();

  asio::deadline_timer m_timer;
  bool m_timer_started;
  std::function<void(void)> m_on_close;
  std::unique_ptr<network::Connection> m_connection;
  Replay m_replay;

  std::uint32_t m_replay_start_ms;
  int m_playback_speed;
};

#endif  // REPLAYSERVER_SRC_REPLAY_CONNECTION_H_
