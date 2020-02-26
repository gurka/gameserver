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

#ifndef TEST_BACKENDMOCK_H_
#define TEST_BACKENDMOCK_H_

#include <cstdint>

#include "gmock/gmock.h"

namespace network
{

struct Backend
{
  enum shutdown_type { shutdown_both = 1 };

  enum Error { no_error = 0, operation_aborted = 1, other_error = 2 };

  struct ErrorCode
  {
    ErrorCode() : val_(0) {}
    ErrorCode(int val) : val_(val) {}

    bool operator==(Error e) const { return val_ == e; }
    operator bool() const { return val_ != 0; }
    std::string message() const { return ""; }

    int val_;
  };

  struct Socket;

  struct Service
  {
    // Calls from Acceptor
    MOCK_METHOD0(acceptor_cancel, void());
    MOCK_METHOD2(acceptor_async_accept, void(Socket&, const std::function<void(const ErrorCode&)>&));

    // Calls from Socket
    MOCK_CONST_METHOD0(socket_is_open, bool());
    MOCK_METHOD2(socket_shutdown, void(shutdown_type, ErrorCode&));
    MOCK_METHOD1(socket_close, void(ErrorCode&));

    // Calls from static functions
    MOCK_METHOD4(async_write, void(Socket&,
                                   const std::uint8_t*,
                                   std::size_t,
                                   const std::function<void(const ErrorCode&, std::size_t)>&));

    MOCK_METHOD4(async_read, void(Socket&,
                                  std::uint8_t*,
                                  std::size_t,
                                  const std::function<void(const ErrorCode&, std::size_t)>&));
  };

  struct Socket
  {
    Socket(Service& service)
      : service_(service)
    {
    }

    bool is_open() const { return service_.socket_is_open(); }
    void shutdown(shutdown_type st, ErrorCode& ec) { service_.socket_shutdown(st, ec); }
    void close(ErrorCode& ec) { service_.socket_close(ec); }

    Service& service_;
  };

  struct Acceptor
  {
    Acceptor(Service& service, int port)
      : service_(service),
        port_(port)
    {
    }

    void cancel() { service_.acceptor_cancel(); }
    void async_accept(Socket& s, const std::function<void(const ErrorCode&)>& cb)
    {
      service_.acceptor_async_accept(s, cb);
    }

    Service& service_;
    int port_;
  };

  static void async_write(Socket& socket,
                          const std::uint8_t* buffer,
                          std::size_t length,
                          const std::function<void(const ErrorCode&, std::size_t)>& handler)
  {
    socket.service_.async_write(socket, buffer, length, handler);
  }

  static void async_read(Socket& socket,
                         std::uint8_t* buffer,
                         std::size_t length,
                         const std::function<void(const ErrorCode&, std::size_t)>& handler)
  {
    socket.service_.async_read(socket, buffer, length, handler);
  }
};

}  // namespace network

#endif  // TEST_BACKENDMOCK_H_
