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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "acceptor.h"

class AcceptorTest : public ::testing::Test
{
 public:
  AcceptorTest()
    : service_(),
      callbacks_({std::bind(&OnAcceptMock::onAccept, &onAcceptMock_, std::placeholders::_1)}),
      onAcceptMock_(),
      acceptor_(&service_, 1234, callbacks_)
  {
  }

  struct Backend
  {
    class Socket;
    class ErrorCode;

    struct Service
    {
      MOCK_METHOD0(acceptor_cancel, void());
      MOCK_METHOD2(async_accept, void(Socket&, const std::function<void(const ErrorCode&)>&));
    };

    class Socket
    {
     public:
      Socket(Service& service)
        : service_(service)
      {
      }

     private:
      Service& service_;
    };

    enum Error { operation_aborted = 1 };

    class ErrorCode
    {
     public:
      ErrorCode(int val) : val_(val) {}

      bool operator==(Error e) const { return val_ == e; }
      operator bool() const { return val_ != 0; }
      std::string message() const { return ""; }

     private:
      int val_;
    };

    class Acceptor
    {
     public:
      Acceptor(Service& service, int port)
        : service_(service),
          port_(port)
      {
      }

      void cancel()
      {
        service_.acceptor_cancel();
      }

      void async_accept(Socket& s, const std::function<void(const ErrorCode&)>& cb)
      {
        service_.async_accept(s, cb);
      }

     private:
      Service& service_;
      int port_;
    };
  };

  struct OnAcceptMock
  {
   public:
    MOCK_METHOD1(onAccept, void(Backend::Socket));
  };

 protected:
  // Helpers
  Backend::Service service_;
  Acceptor<Backend>::Callbacks callbacks_;
  OnAcceptMock onAcceptMock_;

  // Under test
  Acceptor<Backend> acceptor_;
};

TEST_F(AcceptorTest, StartStop)
{
  using ::testing::_;

  // Start async accept
  EXPECT_CALL(service_, async_accept(_, _));
  EXPECT_TRUE(acceptor_.start());
  EXPECT_FALSE(acceptor_.start());

  // Stop Acceptor
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.stop();

  acceptor_.stop();  // Should not have any side effects
}

TEST_F(AcceptorTest, AsyncAccept)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Start async accept, save callback
  std::function<void(Backend::ErrorCode)> callback;
  EXPECT_CALL(service_, async_accept(_, _)).WillOnce(SaveArg<1>(&callback));
  EXPECT_TRUE(acceptor_.start());

  // Call callback with non-error errorcode
  EXPECT_CALL(onAcceptMock_, onAccept(_));
  EXPECT_CALL(service_, async_accept(_, _));  // Acceptor should call async_accept again, to accept next connection
  callback(0);

  // Stop Acceptor
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.stop();
}

TEST_F(AcceptorTest, AsyncAcceptError)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Start async accept, save callback
  std::function<void(Backend::ErrorCode)> callback;
  EXPECT_CALL(service_, async_accept(_, _)).WillOnce(SaveArg<1>(&callback));
  EXPECT_TRUE(acceptor_.start());

  // Call callback with any errorcode except operation_aborted (1)
  EXPECT_CALL(service_, async_accept(_, _));
  callback(49);

  // Call callback with non-error errorcode to make sure new accepts is handled correctly
  EXPECT_CALL(onAcceptMock_, onAccept(_));
  EXPECT_CALL(service_, async_accept(_, _));
  callback(0);

  // Stop Acceptor
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.stop();
}

TEST_F(AcceptorTest, AsyncAcceptAbort)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Start async accept, save callback
  std::function<void(Backend::ErrorCode)> callback;
  EXPECT_CALL(service_, async_accept(_, _)).WillOnce(SaveArg<1>(&callback));
  EXPECT_TRUE(acceptor_.start());

  // Call callback with Error::operation_aborted (1)
  // Acceptor should not call onAccept callback nor start a new async_accept
  callback(Backend::Error::operation_aborted);

  // Stop Acceptor
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.stop();
}
