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

#include "emscripten_client_backend.h"

#include "logger.h"

namespace network
{

EmscriptenClient::EmscriptenClient(emscripten::val ws)
    : m_ws(ws),
      m_read_buffer(),
      m_async_read_buffer(nullptr),
      m_async_read_length(),
      m_async_read_handler()
{
}

bool EmscriptenClient::isConnected() const
{
  return m_ws != emscripten::val::null();
}

void EmscriptenClient::close(ErrorCode& ec)
{
  if (!isConnected())
  {
    LOG_ERROR("%s: called but we are not connected", __func__);
    ec = ErrorCode("Not connected");
    return;
  }

  LOG_DEBUG("%s", __func__);

  ec = ErrorCode();

  // Call close and wait for handleClose to be called before
  // aborting any async reads/writes
  m_ws.call<void>("close");
}

void EmscriptenClient::handleClose()
{
  if (!isConnected())
  {
    LOG_ERROR("%s: called but m_ws is null", __func__);
    return;
  }

  m_ws = emscripten::val::null();
  if (m_async_read_buffer)
  {
    m_async_read_handler(ErrorCode("Connection closed"), 0U);
  }
}

void EmscriptenClient::handleMessage(const std::uint8_t* buffer, std::size_t length)
{
  if (!isConnected())
  {
    LOG_ERROR("%s: called but we are not connected", __func__);
    return;
  }

  //LOG_DEBUG("%s: received %d bytes", __func__, static_cast<int>(msg->get_payload().size()));
  std::copy(buffer, buffer + length, std::back_inserter(m_read_buffer));
  checkAsyncRead();
}

void EmscriptenClient::asyncWrite(const std::uint8_t*,
                                  std::size_t,
                                  const AsyncHandler&)
{
  LOG_ERROR("%s: not yet implemented", __func__);
}

void EmscriptenClient::asyncRead(std::uint8_t* buffer,
                                 std::size_t length,
                                 const AsyncHandler& handler)
{
  if (m_async_read_buffer)
  {
    LOG_ERROR("%s: another async read is already in progress", __func__);
    return;
  }

  m_async_read_buffer = buffer;
  m_async_read_length = length;
  m_async_read_handler = handler;

  // We might already have enough data, so check for it
  checkAsyncRead();
}

void EmscriptenClient::checkAsyncRead()
{
  // Calling the handler might queue another async_read, so continue to check
  // until either the buffer is nullptr (no new async_read made) or we don't have enough
  // data
  /*
  LOG_DEBUG("%s: have buffer=%s, m_read_buffer.size() = %d, m_async_read_length=%d",
            __func__,
            (m_async_read_buffer ? "yes" : "no"),
            static_cast<int>(m_read_buffer.size()),
            static_cast<int>(m_async_read_length));
  */
  while (m_async_read_buffer && m_read_buffer.size() >= m_async_read_length)
  {
    std::copy(m_read_buffer.begin(),
              m_read_buffer.begin() + m_async_read_length,
              m_async_read_buffer);
    m_read_buffer.erase(m_read_buffer.begin(), m_read_buffer.begin() + m_async_read_length);
    m_async_read_buffer = nullptr;
    m_async_read_handler(ErrorCode(), m_async_read_length);
  }
}

void EmscriptenClientBackend::async_write(Socket& socket,
                                          const std::uint8_t* buffer,
                                          std::size_t length,
                                          const EmscriptenClient::AsyncHandler& handler)
{
  socket.client->asyncWrite(buffer, length, handler);
}

void EmscriptenClientBackend::async_read(Socket& socket,
                                         std::uint8_t* buffer,
                                         std::size_t length,
                                         const EmscriptenClient::AsyncHandler& handler)
{
  socket.client->asyncRead(buffer, length, handler);
}

}
