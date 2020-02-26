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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "acceptor.h"
#include "backend_mock.h"

namespace network
{

class AcceptorTest : public ::testing::Test
{
 public:
  AcceptorTest()
    : service_(),
      onAcceptMock_(),
      acceptor_()
  {
  }

  struct OnAcceptMock
  {
    MOCK_METHOD1(onAccept, void(Backend::Socket));
  };

 protected:
  // Helpers
  Backend::Service service_;
  OnAcceptMock onAcceptMock_;

  // Under test
  std::unique_ptr<Acceptor<Backend>> acceptor_;
};

TEST_F(AcceptorTest, CreateDelete)
{
  using ::testing::_;

  // Create Acceptor, should call async_accept
  EXPECT_CALL(service_, acceptor_async_accept(_, _));
  acceptor_ = std::make_unique<Acceptor<Backend>>(&service_, 1234, [this](Backend::Socket socket)
  {
    onAcceptMock_.onAccept(socket);
  });

  // Delete Acceptor, should call cancel
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.reset();
}

TEST_F(AcceptorTest, AsyncAccept)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Start Acceptor, save callback from async_accept
  std::function<void(Backend::ErrorCode)> callback;
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&callback));
  acceptor_ = std::make_unique<Acceptor<Backend>>(&service_, 1234, [this](Backend::Socket socket)
  {
    onAcceptMock_.onAccept(socket);
  });

  // Call callback with non-error errorcode
  EXPECT_CALL(onAcceptMock_, onAccept(_));
  EXPECT_CALL(service_, acceptor_async_accept(_, _));  // Acceptor should call async_accept again, to accept next connection
  callback(Backend::Error::no_error);

  // Delete Acceptor
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.reset();
}

TEST_F(AcceptorTest, AsyncAcceptError)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Create Acceptor, save callback from async_accept
  std::function<void(Backend::ErrorCode)> callback;
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&callback));
  acceptor_ = std::make_unique<Acceptor<Backend>>(&service_, 1234, [this](Backend::Socket socket)
  {
    onAcceptMock_.onAccept(socket);
  });

  // Call callback with any errorcode except operation_aborted (1)
  EXPECT_CALL(service_, acceptor_async_accept(_, _));
  callback(Backend::Error::other_error);

  // Call callback with non-error errorcode to make sure new accepts is handled correctly
  EXPECT_CALL(onAcceptMock_, onAccept(_));
  EXPECT_CALL(service_, acceptor_async_accept(_, _));
  callback(Backend::Error::no_error);

  // Delete Acceptor
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.reset();
}

TEST_F(AcceptorTest, AsyncAcceptAbort)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Create Acceptor, save callback from async_accept
  std::function<void(Backend::ErrorCode)> callback;
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&callback));
  acceptor_ = std::make_unique<Acceptor<Backend>>(&service_, 1234, [this](Backend::Socket socket)
  {
    onAcceptMock_.onAccept(socket);
  });

  // Call callback with Error::operation_aborted (1)
  // Acceptor should not call onAccept callback nor start a new async_accept
  callback(Backend::Error::operation_aborted);

  // Stop Acceptor
  EXPECT_CALL(service_, acceptor_cancel());
  acceptor_.reset();
}

}  // namespace network
