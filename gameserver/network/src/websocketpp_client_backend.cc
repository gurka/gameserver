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

#include "websocketpp_client_backend.h"

#include <memory>

#define ASIO_STANDALONE 1
#include <asio.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "connection_impl.h"
#include "logger.h"

namespace network
{

WebsocketClient::WebsocketClient(websocketpp::client<websocketpp::config::asio_client>::connection_ptr websocketpp_connection)
    : m_websocketpp_connection(websocketpp_connection),
      m_read_buffer(),
      m_async_read_buffer(nullptr),
      m_async_read_length(0U),
      m_async_read_handler()
{
  // Set handlers
  m_websocketpp_connection->set_close_handler([this](websocketpp::connection_hdl)
  {
    handleClose();
  });

  m_websocketpp_connection->set_message_handler([this](websocketpp::connection_hdl,
                                                       websocketpp::client<websocketpp::config::asio_client>::message_ptr msg)
  {
    handleMessage(msg);
  });
}

bool WebsocketClient::isConnected() const
{
  return m_websocketpp_connection->get_state() == websocketpp::session::state::open;
}

void WebsocketClient::close(ErrorCode&)
{
  if (!isConnected())
  {
    LOG_ERROR("%s: called but we are not connected", __func__);
    return;
  }

  // Call close and wait for handleClose to be called before
  // aborting any async reads/writes
  std::error_code ec;
  m_websocketpp_connection->close(websocketpp::close::status::normal, "", ec);
  if (ec)
  {
    LOG_ERROR("%s: closing connection error: %s", __func__, ec.message().c_str());
  }
}

void WebsocketClient::handleClose()
{
  const auto state = m_websocketpp_connection->get_state();
  const auto state_str = state == websocketpp::session::state::connecting ? "connecting" :
                        (state == websocketpp::session::state::open ? "open" :
                        (state == websocketpp::session::state::closing ? "closing" :
                        (state == websocketpp::session::state::closed ? "closed" : "unknown")));
  LOG_DEBUG("%s: current state: %s", __func__, state_str);

  if (state == websocketpp::session::state::closed)
  {
    // Abort any async reads/writes
    if (m_async_read_buffer)
    {
      m_async_read_handler(ErrorCode("Connection closed"), 0U);
    }
  }
}

void WebsocketClient::handleMessage(websocketpp::client<websocketpp::config::asio_client>::message_ptr msg)
{
  if (!isConnected())
  {
    LOG_ERROR("%s: called but we are not connected", __func__);
    return;
  }

  //LOG_DEBUG("%s: received %d bytes", __func__, static_cast<int>(msg->get_payload().size()));
  std::copy(reinterpret_cast<const std::uint8_t*>(msg->get_payload().data()),
            reinterpret_cast<const std::uint8_t*>(msg->get_payload().data() + msg->get_payload().size()),
            std::back_inserter(m_read_buffer));
  checkAsyncRead();
}

void WebsocketClient::asyncWrite(const std::uint8_t*,
                                 std::size_t,
                                 const AsyncHandler&)
{
  LOG_ERROR("%s: not yet implemented", __func__);
}

void WebsocketClient::asyncRead(std::uint8_t* buffer,
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

void WebsocketClient::checkAsyncRead()
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

void WebsocketBackend::async_write(Socket& socket,
                                   const std::uint8_t* buffer,
                                   std::size_t length,
                                   const WebsocketClient::AsyncHandler& handler)
{
  socket.client->asyncWrite(buffer, length, handler);
}

void WebsocketBackend::async_read(Socket& socket,
                                  std::uint8_t* buffer,
                                  std::size_t length,
                                  const WebsocketClient::AsyncHandler& handler)
{
  socket.client->asyncRead(buffer, length, handler);
}

}  // namespace network
