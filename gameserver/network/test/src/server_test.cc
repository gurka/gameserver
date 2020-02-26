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

#include "backend_mock.h"
#include "server_impl.h"

namespace network
{

using ::testing::_;
using ::testing::SaveArg;

class ServerTest : public ::testing::Test
{
 public:
  ServerTest()
    : server_(),
      service_(),
      callbackMock_()
  {
  }

  struct ServerCallbackMock
  {
    MOCK_METHOD1(onClientConnected, void(std::unique_ptr<Connection>&&));
  };

 protected:
  std::unique_ptr<Server> server_;
  Backend::Service service_;
  ServerCallbackMock callbackMock_;
};

TEST_F(ServerTest, AcceptConnection)
{
  std::function<void(const Backend::ErrorCode&)> onAcceptHandler;

  // Create Server, should call async_accept
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&onAcceptHandler));
  server_ = std::make_unique<ServerImpl<Backend>>(&service_, 1234, [this](std::unique_ptr<Connection>&& connection)
  {
    callbackMock_.onClientConnected(std::move(connection));
  });

  // Call onAccept handler with non-error errorcode
  // Server should call onClientConnected callback and call async_accept again
  EXPECT_CALL(callbackMock_, onClientConnected(_));
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&onAcceptHandler));
  onAcceptHandler(Backend::Error::no_error);

  // Call onAccept handler with errorcode
  // Server should not call onClientConnected, but continue to accept
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&onAcceptHandler));
  onAcceptHandler(Backend::Error::other_error);

  // Delete Server, should call cancel
  EXPECT_CALL(service_, acceptor_cancel());
  server_.reset();
}

}  // namespace network
