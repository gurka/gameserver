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

#include "websocket_server_impl.h"

#include "logger.h"

namespace network
{

bool WebsocketBackend::Socket::is_open() const  // NOLINT
{
  return static_cast<bool>(hdl.lock());
}

void WebsocketBackend::Socket::shutdown(shutdown_type type, ErrorCode& ec)
{
  (void)type;
  ec = WebsocketBackend::ErrorCode();
}

void WebsocketBackend::Socket::close(ErrorCode& ec)
{
  server->close(hdl, ec);
}

void WebsocketBackend::async_read(Socket socket,  // NOLINT
                                  std::uint8_t* buffer,
                                  unsigned length,
                                  const std::function<void(ErrorCode, std::size_t)>& callback)
{
  socket.server->asyncRead(socket, buffer, length, callback);
}

void WebsocketBackend::async_write(Socket socket,  // NOLINT
                                   const std::uint8_t* buffer,
                                   unsigned length,
                                   const std::function<void(ErrorCode, std::size_t)>& callback)
{
  socket.server->asyncWrite(socket, buffer, length, callback);
}

WebsocketServerImpl::WebsocketServerImpl(asio::io_context* io_context,
                                         int port,
                                         std::function<void(std::unique_ptr<Connection>&&)> on_client_connected)
    : m_on_client_connected(std::move(on_client_connected))
{

  websocketpp::lib::error_code ec;
  m_server.init_asio(io_context, ec);
  if (ec)
  {
    LOG_ERROR("%s: could not initialize WebsocketServer: %s", __func__, ec.message().c_str());
    return;
  }

  // Disable all websocketpp logging for now
  m_server.set_access_channels(websocketpp::log::alevel::none);

  m_server.set_reuse_addr(true);
  m_server.set_open_handler([this](const websocketpp::connection_hdl& hdl)
  {
    LOG_DEBUG("%s: new connection", __func__);

    auto socket = WebsocketBackend::Socket();
    socket.server = this;
    socket.hdl = hdl;
    m_on_client_connected(std::make_unique<ConnectionImpl<WebsocketBackend>>(std::move(socket)));
  });

  m_server.set_close_handler([this](const websocketpp::connection_hdl& hdl)
  {
    closeConnection(hdl);
  });

  m_server.set_message_handler([this](const websocketpp::connection_hdl& hdl,
                                      const WebsocketServer::message_ptr& msg)
  {
    LOG_DEBUG("%s: new message", __func__);

    const auto hdl_lock = hdl.lock();

    // Add to BufferedData
    auto it = std::find_if(m_buffered_data.begin(),
                           m_buffered_data.end(),
                           [&hdl_lock](const BufferedData& data) { return data.hdl.lock() == hdl_lock; });
    if (it == m_buffered_data.end())
    {
      // Create new BufferedData
      m_buffered_data.emplace_back(hdl);
      it = m_buffered_data.end() - 1;
    }

    auto& buffered_data = *it;
    buffered_data.payload.insert(buffered_data.payload.begin(),
                                 reinterpret_cast<const std::uint8_t*>(msg->get_payload().c_str()),
                                 reinterpret_cast<const std::uint8_t*>(msg->get_payload().c_str()) + msg->get_payload().length());

    // Call fix, which might forward the data if there is a AsyncRead waiting and we have enough data
    fix(hdl_lock);
  });

  m_server.listen(port);
  m_server.start_accept();
}

void WebsocketServerImpl::asyncRead(const WebsocketBackend::Socket& socket,
                                    std::uint8_t* buffer,
                                    unsigned length,
                                    const std::function<void(WebsocketBackend::ErrorCode, std::size_t)>& callback)
{
  LOG_DEBUG("%s: new async_read with length: %u", __func__, length);

  // Add the AsyncRead
  m_async_reads.emplace_back(socket.hdl, buffer, length, callback);

  // Call fix, which might send a response if we have enough buffered data
  fix(socket.hdl.lock());
}

void WebsocketServerImpl::asyncWrite(const WebsocketBackend::Socket& socket,
                                     const std::uint8_t* buffer,
                                     unsigned length,
                                     const std::function<void(WebsocketBackend::ErrorCode, std::size_t)>& callback)
{
  websocketpp::lib::error_code error;
  m_server.send(socket.hdl, buffer, length, websocketpp::frame::opcode::BINARY, error);
  if (error)
  {
    callback(WebsocketBackend::ErrorCode(error.message()), 0U);
  }
  else
  {
    callback(WebsocketBackend::ErrorCode(), length);
  }
}

void WebsocketServerImpl::close(const websocketpp::connection_hdl& hdl, WebsocketBackend::ErrorCode& ec)
{
  // Close socket
  websocketpp::lib::error_code websocketpp_ec;
  m_server.close(hdl, websocketpp::close::status::normal, "Closing connection", websocketpp_ec);
  if (websocketpp_ec)
  {
    ec = WebsocketBackend::ErrorCode(websocketpp_ec.message());
  }
  else
  {
    ec = WebsocketBackend::ErrorCode();
  }

  closeConnection(hdl);
}

void WebsocketServerImpl::closeConnection(const websocketpp::connection_hdl& hdl)
{
  LOG_DEBUG("%s", __func__);

  // Abort and delete AsyncRead if any ongoing
  const auto hdl_lock = hdl.lock();
  const auto ait = std::find_if(m_async_reads.begin(),
                                m_async_reads.end(),
                                [&hdl_lock](const AsyncRead& read) { return read.hdl.lock() == hdl_lock; });
  if (ait != m_async_reads.end())
  {
    const auto read = *ait;
    m_async_reads.erase(ait);
    LOG_DEBUG("%s: aborting AsyncRead", __func__);
    read.callback(WebsocketBackend::ErrorCode("Connection closed"), 0U);
  }

  // Delete BufferedData if any
  const auto bit = std::find_if(m_buffered_data.begin(),
                                m_buffered_data.end(),
                                [&hdl_lock](const BufferedData& data) { return data.hdl.lock() == hdl_lock; });
  if (bit != m_buffered_data.end())
  {
    m_buffered_data.erase(bit);
  }
}

void WebsocketServerImpl::fix(websocketpp::lib::shared_ptr<void> hdl_lock)
{
  // Check if there is any AsyncRead for this connection
  const auto ait = std::find_if(m_async_reads.begin(),
                                m_async_reads.end(),
                                [&hdl_lock](const AsyncRead& read) { return read.hdl.lock() == hdl_lock; });
  if (ait == m_async_reads.end())
  {
    return;
  }
  auto& read = *ait;

  // Check if there is any BufferedData for this connection
  const auto bit = std::find_if(m_buffered_data.begin(),
                                m_buffered_data.end(),
                                [&hdl_lock](const BufferedData& data) { return data.hdl.lock() == hdl_lock; });
  if (bit == m_buffered_data.end())
  {
    return;
  }
  auto& data = *bit;

  // Check if we have enough data to send
  if (data.payload.size() >= read.length)
  {
    // Copy from payload to async_read buffer, then delete it from payload
    std::copy(data.payload.begin(), data.payload.begin() + read.length, read.buffer);
    data.payload.erase(data.payload.begin(), data.payload.begin() + read.length);

    // Make sure to delete AsyncRead before calling callback, as a new async_read call
    // can be made within the callback
    const auto callback = read.callback;
    const auto length = read.length;
    m_async_reads.erase(ait);

    LOG_DEBUG("%s: forwarding data to async_read call with length: %u", __func__, length);

    callback(WebsocketBackend::ErrorCode(), length);
  }
}

}  // namespace network
