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

#include "server_factory.h"

#include <asio.hpp>

#include "server_impl.h"
#include "websocket_server_impl.h"

namespace network
{

struct Backend
{
  using Service = asio::io_context;

  class Acceptor : public asio::ip::tcp::acceptor
  {
   public:
    Acceptor(Service& io_context, int port)  //NOLINT
      : asio::ip::tcp::acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {
    }
  };

  using Socket = asio::ip::tcp::socket;
  using ErrorCode = std::error_code;
  using Error = asio::error::basic_errors;
  using shutdown_type = asio::ip::tcp::socket::shutdown_type;

  static void async_write(Socket& socket,  //NOLINT
                          const std::uint8_t* buffer,
                          std::size_t length,
                          const std::function<void(const Backend::ErrorCode&, std::size_t)>& handler)
  {
    asio::async_write(socket, asio::buffer(buffer, length), handler);
  }

  static void async_read(Socket& socket,  //NOLINT
                         std::uint8_t* buffer,
                         std::size_t length,
                         const std::function<void(const Backend::ErrorCode&, std::size_t)>& handler)
  {
    asio::async_read(socket, asio::buffer(buffer, length), handler);
  }
};

std::unique_ptr<Server> ServerFactory::createServer(asio::io_context* io_context,
                                                    int port,
                                                    const OnClientConnectedCallback& on_client_connected)
{
  return std::make_unique<ServerImpl<Backend>>(io_context, port, on_client_connected);
}

std::unique_ptr<Server> ServerFactory::createWebsocketServer(asio::io_context* io_context,
                                                             int port,
                                                             const OnClientConnectedCallback& on_client_connected)
{
  return std::make_unique<WebsocketServerImpl>(io_context, port, on_client_connected);
}

}  // namespace network
