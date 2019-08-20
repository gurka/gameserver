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

#ifndef NETWORK_SRC_ACCEPTOR_H_
#define NETWORK_SRC_ACCEPTOR_H_

#include <functional>
#include <memory>
#include <utility>

#include "logger.h"

template <typename Backend>
class Acceptor
{
 public:
  Acceptor(typename Backend::Service* io_context,
           int port,
           const std::function<void(typename Backend::Socket&&)>& onAccept)
    : acceptor_(*io_context, port),
      socket_(*io_context),
      onAccept_(onAccept)
  {
    accept();
  }

  virtual ~Acceptor()
  {
    acceptor_.cancel();
  }

  // Delete copy constructors
  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

 private:
  void accept()
  {
    acceptor_.async_accept(socket_, [this](const typename Backend::ErrorCode& errorCode)
    {
      if (errorCode == Backend::Error::operation_aborted)
      {
        // This instance might be deleted, so don't touch any instance variables
        return;
      }
      else if (errorCode)
      {
        LOG_DEBUG("Could not accept connection: %s", errorCode.message().c_str());
      }
      else
      {
        LOG_INFO("Accepted connection");
        onAccept_(std::move(socket_));
      }

      // Continue to accept new connections
      accept();
    });
  }

  typename Backend::Acceptor acceptor_;
  typename Backend::Socket socket_;
  std::function<void(typename Backend::Socket&&)> onAccept_;
};

#endif  // NETWORK_SRC_ACCEPTOR_H_
