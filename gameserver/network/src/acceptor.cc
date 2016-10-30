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

#include "acceptor.h"

#include <utility>

#include "logger.h"

namespace BIP = boost::asio::ip;

Acceptor::Acceptor(boost::asio::io_service* io_service,
                   unsigned short port,
                   const Callbacks& callbacks)
  : acceptor_(*io_service, BIP::tcp::endpoint(BIP::tcp::v4(), port)),
    socket_(*io_service),
    callbacks_(callbacks),
    state_(CLOSED)
{
}

Acceptor::~Acceptor()
{
  if (isListening())
  {
    stop();
  }
}

bool Acceptor::start()
{
  if (state_ != CLOSED)
  {
    LOG_ERROR("Acceptor already starting or currently shutting down");
    return false;
  }
  else
  {
    LOG_INFO("Starting server");
    state_ = LISTENING;
    asyncAccept();
    return true;
  }
}

void Acceptor::stop()
{
  if (state_ != LISTENING)
  {
    LOG_ERROR("Acceptor already stopped or currently shutting down");
  }
  else
  {
    LOG_INFO("Stopping server");
    state_ = CLOSING;
    acceptor_.cancel();
  }
}

void Acceptor::asyncAccept()
{
  acceptor_.async_accept(socket_, [this](const boost::system::error_code& errorCode)
  {
    if (!errorCode)
    {
      LOG_INFO("Accepted connection");
      callbacks_.onAccept(std::move(socket_));
    }
    else
    {
      LOG_DEBUG("Could not accept connection: %s", errorCode.message().c_str());
    }

    if (state_ == LISTENING)
    {
      asyncAccept();
    }
    else  // state == CLOSING || state == CLOSED
    {
      LOG_INFO("Acceptor stopped");
      state_ = CLOSED;
    }
  });
}
