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

#ifndef NETWORK_SRC_SERVER_IMPL_H_
#define NETWORK_SRC_SERVER_IMPL_H_

#include "server.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "acceptor.h"
#include "connection_impl.h"
#include "logger.h"

namespace network
{

template <typename Backend>
class ServerImpl : public Server
{
 public:
  ServerImpl(typename Backend::Service* io_context,
             int port,
             const std::function<void(std::unique_ptr<Connection>&&)>& on_client_connected)
      : m_acceptor(io_context,
                   port,
                   [on_client_connected](typename Backend::Socket&& socket)
                   {
                     LOG_DEBUG("onAccept()");
                     on_client_connected(std::make_unique<ConnectionImpl<Backend>>(std::move(socket)));
                   })
  {
  }

  // Delete copy constructors
  ServerImpl(const ServerImpl&) = delete;
  ServerImpl& operator=(const ServerImpl&) = delete;

 private:
  Acceptor<Backend> m_acceptor;
};

}  // namespace network

#endif  // NETWORK_SRC_SERVER_IMPL_H_
