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

#ifndef NETWORK_SRC_WEBSOCKET_SERVER_IMPL_H_
#define NETWORK_SRC_WEBSOCKET_SERVER_IMPL_H_

#include "server.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#define ASIO_STANDALONE 1

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "connection_impl.h"
#include "logger.h"

namespace network
{

class WebsocketServerImpl;

struct WebsocketBackend
{
  struct ErrorCode
  {
    ErrorCode() : msg("") {}
    explicit ErrorCode(std::string msg) : error(true), msg(std::move(msg)) {}

    explicit operator bool() const { return error; }
    std::string message() const { return msg; }

    bool error{false};
    std::string msg;
  };

  enum shutdown_type
  {
    shutdown_both
  };

  struct Socket
  {
    WebsocketServerImpl* server;
    websocketpp::connection_hdl hdl;

    bool is_open() const;  // NOLINT
    static void shutdown(shutdown_type, ErrorCode&);  // NOLINT
    void close(ErrorCode&);  // NOLINT
  };

  static void async_read(Socket socket,  // NOLINT
                         std::uint8_t* buffer,
                         unsigned length,
                         const std::function<void(ErrorCode, std::size_t)>& callback);

  static void async_write(Socket socket,  // NOLINT
                          const std::uint8_t* buffer,
                          unsigned length,
                          const std::function<void(ErrorCode, std::size_t)>& callback);
};

class WebsocketServerImpl : public Server
{
  using WebsocketServer = websocketpp::server<websocketpp::config::asio>;

 public:
  WebsocketServerImpl(asio::io_context* io_context,
                      int port,
                      std::function<void(std::unique_ptr<Connection>&&)> on_client_connected);

  ~WebsocketServerImpl() override
  {
    m_server.stop_listening();
  }

  // Called from static functions in WebsocketBackend
  void asyncRead(const WebsocketBackend::Socket& socket,
                 std::uint8_t* buffer,
                 unsigned length,
                 const std::function<void(WebsocketBackend::ErrorCode, std::size_t)>& callback);

  void asyncWrite(const WebsocketBackend::Socket& socket,
                  const std::uint8_t* buffer,
                  unsigned length,
                  const std::function<void(WebsocketBackend::ErrorCode, std::size_t)>& callback);

  // Called from functions in WebsocketBackend::Socket
  void close(const websocketpp::connection_hdl& hdl, WebsocketBackend::ErrorCode& ec);  // NOLINT

 private:
  void closeConnection(const websocketpp::connection_hdl& hdl);
  void fix(websocketpp::lib::shared_ptr<void> hdl_lock);

  WebsocketServer m_server;
  std::function<void(std::unique_ptr<Connection>&&)> m_on_client_connected;

  struct AsyncRead
  {
    AsyncRead(websocketpp::connection_hdl hdl,
              uint8_t* buffer,
              unsigned length,
              std::function<void(WebsocketBackend::ErrorCode, std::size_t)> callback)
        : hdl(std::move(hdl)),
          buffer(buffer),
          length(length),
          callback(std::move(callback))
    {
    }

    websocketpp::connection_hdl hdl;
    uint8_t* buffer;
    unsigned length;
    std::function<void(WebsocketBackend::ErrorCode, std::size_t)> callback;
  };
  std::vector<AsyncRead> m_async_reads;

  struct BufferedData
  {
    explicit BufferedData(websocketpp::connection_hdl hdl)
        : hdl(std::move(hdl))
    {
    }

    websocketpp::connection_hdl hdl;
    std::vector<std::uint8_t> payload;
  };
  std::vector<BufferedData> m_buffered_data;
};

}  // namespace network

#endif  // NETWORK_SRC_WEBSOCKET_SERVER_IMPL_H_
