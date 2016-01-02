/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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
#include <boost/asio.hpp>  //NOLINT

class Acceptor
{
 public:
  struct Callbacks
  {
    std::function<void(boost::asio::ip::tcp::socket socket)> onAccept;
  };

  Acceptor(boost::asio::io_service* io_service,
           unsigned short port,
           const Callbacks& callbacks);
  virtual ~Acceptor();

  // Delete copy constructors
  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

  bool start();
  void stop();
  bool isListening() const { return state_ == LISTENING; }

 private:
  void asyncAccept();

  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;
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
