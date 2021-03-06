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

#ifndef NETWORK_EXPORT_CLIENT_FACTORY_H_
#define NETWORK_EXPORT_CLIENT_FACTORY_H_

#include <functional>
#include <memory>

#ifndef EMSCRIPTEN
namespace asio
{
class io_context;
}
#endif

namespace network
{

class Connection;

class ClientFactory
{
 public:
  struct Callbacks
  {
    std::function<void(std::unique_ptr<Connection>&&)> on_connected;
    std::function<void(void)> on_connect_failure;
  };

#ifndef EMSCRIPTEN
  static bool createWebsocketClient(asio::io_context* io_context, const std::string& uri, const Callbacks& callbacks);
#else
  static bool createWebsocketClient(const std::string& uri, const Callbacks& callbacks);
#endif
};

}  // namespace network

#endif  // NETWORK_EXPORT_CLIENT_FACTORY_H_
