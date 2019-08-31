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

#ifndef NETWORK_SRC_WEBSOCKET_SERVER_IMPL_H_
#define NETWORK_SRC_WEBSOCKET_SERVER_IMPL_H_

#include "server.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "connection_impl.h"
#include "logger.h"

class WebsocketServerImpl;

struct WebsocketBackend
{
  struct ErrorCode
  {
    ErrorCode() : error_(false), msg_("") {}
    ErrorCode(const std::string& msg) : error_(true), msg_(msg) {}

    operator bool() const { return error_; }
    std::string message() const { return msg_; }

    bool error_;
    std::string msg_;
  };

  enum shutdown_type
  {
    shutdown_both
  };

  struct Socket
  {
    WebsocketServerImpl* server;
    websocketpp::connection_hdl hdl;

    bool is_open() const;
    void shutdown(shutdown_type, ErrorCode&);
    void close(ErrorCode&);
  };

  static void async_read(Socket socket,
                         std::uint8_t* buffer,
                         unsigned length,
                         const std::function<void(ErrorCode, std::size_t)> callback);

  static void async_write(Socket socket,
                          const std::uint8_t* buffer,
                          unsigned length,
                          const std::function<void(ErrorCode, std::size_t)> callback);
};

class WebsocketServerImpl : public Server
{
  using WebsocketServer = websocketpp::server<websocketpp::config::asio>;

 public:
  WebsocketServerImpl(boost::asio::io_context* io_context,
                      int port,
                      const std::function<void(std::unique_ptr<Connection>&&)>& onClientConnected);

  ~WebsocketServerImpl()
  {
    server_.stop_listening();
  }

  // Called from static functions in WebsocketBackend
  void async_read(WebsocketBackend::Socket socket,
                  std::uint8_t* buffer,
                  unsigned length,
                  const std::function<void(WebsocketBackend::ErrorCode, std::size_t)> callback);

  void async_write(WebsocketBackend::Socket socket,
                   const std::uint8_t* buffer,
                   unsigned length,
                   const std::function<void(WebsocketBackend::ErrorCode, std::size_t)> callback);

  // Called from functions in WebsocketBackend::Socket
  bool is_open(websocketpp::connection_hdl hdl) const;
  void shutdown(websocketpp::connection_hdl hdl, WebsocketBackend::ErrorCode& ec);
  void close(websocketpp::connection_hdl hdl, WebsocketBackend::ErrorCode& ec);

 private:
  void fix(websocketpp::lib::shared_ptr<void> hdl_lock);

  WebsocketServer server_;
  std::function<void(std::unique_ptr<Connection>&&)> onClientConnected_;

  struct AsyncRead
  {
    AsyncRead(websocketpp::connection_hdl hdl,
              uint8_t* buffer,
              unsigned length,
              const std::function<void(WebsocketBackend::ErrorCode, std::size_t)>& callback)
        : hdl(hdl),
          buffer(buffer),
          length(length),
          callback(callback)
    {
    }

    websocketpp::connection_hdl hdl;
    uint8_t* buffer;
    unsigned length;
    std::function<void(WebsocketBackend::ErrorCode, std::size_t)> callback;
  };
  std::vector<AsyncRead> asyncReads_;

  struct BufferedData
  {
    BufferedData(websocketpp::connection_hdl hdl) : hdl(hdl), payload() {}

    websocketpp::connection_hdl hdl;
    std::vector<std::uint8_t> payload;
  };
  std::vector<BufferedData> bufferedData_;
};

#endif  // NETWORK_SRC_WEBSOCKET_SERVER_IMPL_H_
