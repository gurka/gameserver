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

#ifndef NETWORK_SRC_WEBSOCKETPP_CLIENT_BACKEND_H_
#define NETWORK_SRC_WEBSOCKETPP_CLIENT_BACKEND_H_

#include <cstdint>
#include <memory>
#include <vector>

#define ASIO_STANDALONE 1
#include <asio.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "client_factory.h"
#include "error_code.h"
#include "connection.h"

namespace network
{

class WebsocketClient
{
 public:
  using AsyncHandler = std::function<void(const ErrorCode&, std::size_t)>;

  explicit WebsocketClient(websocketpp::client<websocketpp::config::asio_client>::connection_ptr websocketpp_connection);

  // Used by ClientFactory
  websocketpp::client<websocketpp::config::asio_client>::connection_ptr getWebsocketppConnection() { return m_websocketpp_connection; }

  // Required from WebsocketBackend::Socket
  bool isConnected() const;
  void close(ErrorCode* ec);

  // websocketpp connection handlers
  void handleClose();
  void handleMessage(const websocketpp::client<websocketpp::config::asio_client>::message_ptr& msg);

  // Called from static functions in WebsocketppBackend
  void asyncWrite(const std::uint8_t* buffer,
                  std::size_t length,
                  const AsyncHandler& handler);
  void asyncRead(std::uint8_t* buffer,
                 std::size_t length,
                 const AsyncHandler& handler);

 private:
  void checkAsyncRead();

  websocketpp::client<websocketpp::config::asio_client>::connection_ptr m_websocketpp_connection;

  std::vector<std::uint8_t> m_read_buffer;
  std::uint8_t* m_async_read_buffer;
  std::size_t m_async_read_length;
  AsyncHandler m_async_read_handler;
};

struct WebsocketBackend
{
  using shutdown_type = asio::ip::tcp::socket::shutdown_type;

  using ErrorCode = ErrorCode;

  struct Socket
  {
    explicit Socket(std::unique_ptr<WebsocketClient>&& client)
        : client(std::move(client))
    {
    }

    bool is_open() const { return client->isConnected(); }  // NOLINT
    void shutdown(shutdown_type, ErrorCode&) {}  // NOLINT
    void close(ErrorCode& ec) { client->close(&ec); }  // NOLINT

    std::unique_ptr<WebsocketClient> client;
  };

  static void async_write(Socket& socket,  // NOLINT
                          const std::uint8_t* buffer,
                          std::size_t length,
                          const WebsocketClient::AsyncHandler& handler);

  static void async_read(Socket& socket,  // NOLINT
                         std::uint8_t* buffer,
                         std::size_t length,
                         const WebsocketClient::AsyncHandler& handler);
};

}  // namespace network

#endif  // NETWORK_SRC_WEBSOCKETPP_CLIENT_BACKEND_H_
