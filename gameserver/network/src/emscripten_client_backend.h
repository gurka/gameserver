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

#ifndef NETWORK_SRC_EMSCRIPTEN_CLIENT_BACKEND_H_
#define NETWORK_SRC_EMSCRIPTEN_CLIENT_BACKEND_H_

#include <cstdint>
#include <memory>
#include <vector>

#include <emscripten.h>
#include <emscripten/val.h>

#include "client_factory.h"
#include "error_code.h"
#include "connection.h"

namespace network
{

class EmscriptenClient
{
 public:
  using AsyncHandler = std::function<void(const ErrorCode&, std::size_t)>;

  EmscriptenClient(emscripten::val ws);

  // Required from WebsocketBackend::Socket
  bool isConnected() const;
  void shutdown(ErrorCode& ec) { ec = ErrorCode(); }
  void close(ErrorCode& ec);

  // websocketpp connection handlers
  void handleClose();
  void handleMessage(const std::uint8_t* buffer, std::size_t length);

  // Called from static functions in WebsocketppBackend
  void asyncWrite(const std::uint8_t* buffer,
                  std::size_t length,
                  const AsyncHandler& handler);
  void asyncRead(std::uint8_t* buffer,
                 std::size_t length,
                 const AsyncHandler& handler);

 private:
  void checkAsyncRead();

  emscripten::val m_ws;

  std::vector<std::uint8_t> m_read_buffer;
  std::uint8_t* m_async_read_buffer;
  std::size_t m_async_read_length;
  AsyncHandler m_async_read_handler;
};

struct EmscriptenClientBackend
{
  enum shutdown_type
  {
    shutdown_both
  };

  using ErrorCode = ErrorCode;

  struct Socket
  {
    Socket(std::unique_ptr<EmscriptenClient>&& client)
        : client(std::move(client))
    {
    }

    bool is_open() const { return client->isConnected(); }
    void shutdown(shutdown_type, ErrorCode& ec) { client->shutdown(ec); }
    void close(ErrorCode& ec) { client->close(ec); }

    std::unique_ptr<EmscriptenClient> client;
  };

  static void async_write(Socket& socket,
                          const std::uint8_t* buffer,
                          std::size_t length,
                          const EmscriptenClient::AsyncHandler& handler);

  static void async_read(Socket& socket,
                         std::uint8_t* buffer,
                         std::size_t length,
                         const EmscriptenClient::AsyncHandler& handler);
};

}  // namespace network

#endif  // NETWORK_SRC_EMSCRIPTEN_CLIENT_BACKEND_H_
