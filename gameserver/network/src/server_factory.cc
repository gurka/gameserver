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

#include "server_factory.h"

#include <boost/asio.hpp>  //NOLINT

#include "server_impl.h"

struct Backend
{
  using Service = boost::asio::io_service;

  class Acceptor : public boost::asio::ip::tcp::acceptor
  {
   public:
    Acceptor(Service& io_service, int port)  //NOLINT
      : boost::asio::ip::tcp::acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    {
    }
  };

  using Socket = boost::asio::ip::tcp::socket;
  using ErrorCode = boost::system::error_code;
  using Error = boost::asio::error::basic_errors;
  using shutdown_type = boost::asio::ip::tcp::socket::shutdown_type;

  static void async_write(Socket& socket,  //NOLINT
                          const std::uint8_t* buffer,
                          std::size_t length,
                          const std::function<void(const Backend::ErrorCode&, std::size_t)>& handler)
  {
    boost::asio::async_write(socket, boost::asio::buffer(buffer, length), handler);
  }

  static void async_read(Socket& socket,  //NOLINT
                         std::uint8_t* buffer,
                         std::size_t length,
                         const std::function<void(const Backend::ErrorCode&, std::size_t)>& handler)
  {
    boost::asio::async_read(socket, boost::asio::buffer(buffer, length), handler);
  }
};

std::unique_ptr<Server> ServerFactory::createServer(boost::asio::io_service* io_service,
                                                    unsigned short port,
                                                    const Server::Callbacks& callbacks)
{
  return std::make_unique<ServerImpl<Backend>>(io_service, port, callbacks);
}
