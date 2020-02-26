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

#ifndef NETWORK_SRC_ACCEPTOR_H_
#define NETWORK_SRC_ACCEPTOR_H_

#include <functional>
#include <memory>
#include <utility>

#include "logger.h"

namespace network
{

template <typename Backend>
class Acceptor
{
 public:
  Acceptor(typename Backend::Service* io_context,
           int port,
           std::function<void(typename Backend::Socket&&)> on_accept)
    : m_acceptor(*io_context, port),
      m_socket(*io_context),
      m_on_accept(std::move(on_accept))
  {
    accept();
  }

  virtual ~Acceptor()
  {
    m_acceptor.cancel();
  }

  // Delete copy constructors
  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

 private:
  void accept()
  {
    m_acceptor.async_accept(m_socket, [this](const typename Backend::ErrorCode& error_code)
    {
      if (error_code == Backend::Error::operation_aborted)
      {
        // This instance might be deleted, so don't touch any instance variables
        return;
      }

      if (error_code)
      {
        LOG_DEBUG("Could not accept connection: %s", error_code.message().c_str());
      }
      else
      {
        LOG_INFO("Accepted connection");
        m_on_accept(std::move(m_socket));
      }

      // Continue to accept new connections
      accept();
    });
  }

  typename Backend::Acceptor m_acceptor;
  typename Backend::Socket m_socket;
  std::function<void(typename Backend::Socket&&)> m_on_accept;
};

}  // namespace network

#endif  // NETWORK_SRC_ACCEPTOR_H_
