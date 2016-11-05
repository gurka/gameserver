/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
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

#ifndef NETWORK_ACCEPTOR_H_
#define NETWORK_ACCEPTOR_H_

#include <functional>
#include <memory>
#include <utility>

#include "logger.h"

template <typename Backend>
class Acceptor
{
 public:
  using BEAcceptor = typename Backend::Acceptor;
  using Service    = typename Backend::Service;
  using Socket     = typename Backend::Socket;
  using ErrorCode  = typename Backend::ErrorCode;
  using Error      = typename Backend::Error;

  struct Callbacks
  {
    std::function<void(Socket)> onAccept;
  };

  Acceptor(Service* io_service,
           unsigned short port,
           const Callbacks& callbacks)
    : acceptor_(*io_service, port),
      socket_(*io_service),
      callbacks_(callbacks),
      state_(CLOSED)
  {
  }

  virtual ~Acceptor()
  {
    if (isListening())
    {
      stop();
    }
  }

  // Delete copy constructors
  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

  bool start()
  {
    if (state_ != CLOSED)
    {
      LOG_ERROR("Acceptor already starting or currently shutting down");
      return false;
    }
    else
    {
      LOG_INFO("Starting Acceptor");
      state_ = LISTENING;
      accept();
      return true;
    }
  }

  void stop()
  {
    if (state_ != LISTENING)
    {
      LOG_ERROR("Acceptor already stopped or currently shutting down");
    }
    else
    {
      LOG_INFO("Stopping Acceptor");
      state_ = CLOSING;
      acceptor_.cancel();
    }
  }

  bool isListening() const { return state_ == LISTENING; }

 private:
  void accept()
  {
    acceptor_.async_accept(socket_, std::bind(&Acceptor::onAccept, this, std::placeholders::_1));
  }

  void onAccept(const ErrorCode& errorCode)
  {
    if (errorCode == Error::operation_aborted)
    {
      return;
    }
    else if (errorCode)
    {
      LOG_DEBUG("Could not accept connection: %s", errorCode.message().c_str());
    }
    else
    {
      LOG_INFO("Accepted connection");
      callbacks_.onAccept(std::move(socket_));
    }

    if (state_ == LISTENING)
    {
      accept();
    }
    else  // state == CLOSING || state == CLOSED
    {
      // We should never really get here
      // If we cancel the Acceptor this function will be called with
      // Error::operation_aborted, which is catched earlier in this function
      LOG_INFO("Acceptor stopped");
      state_ = CLOSED;
    }
  }

  BEAcceptor acceptor_;
  Socket socket_;
  Callbacks callbacks_;

  enum State
  {
    CLOSED,
    LISTENING,
    CLOSING,
  };
  State state_;
};

#endif  // NETWORK_ACCEPTOR_H_
