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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "backend_mock.h"
#include "server_impl.h"

class ServerTest : public ::testing::Test
{
 public:
  ServerTest()
    : server_(),
      service_(),
      callbacksMock_(),
      callbacks_({
        std::bind(&ServerCallbackMock::onClientConnected, &callbacksMock_, std::placeholders::_1),
        std::bind(&ServerCallbackMock::onClientDisconnected, &callbacksMock_, std::placeholders::_1),
        std::bind(&ServerCallbackMock::onPacketReceived, &callbacksMock_, std::placeholders::_1, std::placeholders::_2),
      })
  {
  }

  struct ServerCallbackMock
  {
    MOCK_METHOD1(onClientConnected, void(ConnectionId));
    MOCK_METHOD1(onClientDisconnected, void(ConnectionId));
    MOCK_METHOD2(onPacketReceived, void(ConnectionId, IncomingPacket*));
  };

 protected:
  std::unique_ptr<Server> server_;
  Backend::Service service_;
  ServerCallbackMock callbacksMock_;
  Server::Callbacks callbacks_;
};

TEST_F(ServerTest, CreateDelete)
{
  using ::testing::_;

  // Create Server, should call async_accept
  EXPECT_CALL(service_, acceptor_async_accept(_, _));
  server_ = std::make_unique<ServerImpl<Backend>>(&service_, 1234, callbacks_);

  // Delete Server, should call cancel
  EXPECT_CALL(service_, acceptor_cancel());
  server_.reset();
}

TEST_F(ServerTest, AcceptAndCloseConnection)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Create Server, save onAccept handler
  std::function<void(const Backend::ErrorCode&)> onAcceptHandler;
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&onAcceptHandler));
  server_ = std::make_unique<ServerImpl<Backend>>(&service_, 1234, callbacks_);

  // Call onAccept handler with non-error errorcode
  // Server should call onClientConnected callback and call async_accept again
  // Connection should call async_read
  ConnectionId connectionId;
  EXPECT_CALL(callbacksMock_, onClientConnected(_)).WillOnce(SaveArg<0>(&connectionId));
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&onAcceptHandler));
  EXPECT_CALL(service_, async_read(_, _, _, _));
  onAcceptHandler(Backend::Error::no_error);

  // Close the Connection with force = false, connection should call socket_shutdown and socket_close
  EXPECT_CALL(service_, socket_shutdown(_, _));
  EXPECT_CALL(service_, socket_close(_));
  server_->closeConnection(connectionId, false);

  // Do the same thing again but close the connection with force = true (should not be any difference)
  EXPECT_CALL(callbacksMock_, onClientConnected(_)).WillOnce(SaveArg<0>(&connectionId));
  EXPECT_CALL(service_, acceptor_async_accept(_, _));
  EXPECT_CALL(service_, async_read(_, _, _, _));
  onAcceptHandler(Backend::Error::no_error);

  EXPECT_CALL(service_, socket_shutdown(_, _));
  EXPECT_CALL(service_, socket_close(_));
  server_->closeConnection(connectionId, true);

  // Delete Server, should call cancel
  EXPECT_CALL(service_, acceptor_cancel());
  server_.reset();
}

TEST_F(ServerTest, UnexpectedDisconnect)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Create Server, save onAccept handler
  std::function<void(const Backend::ErrorCode&)> onAcceptHandler;
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&onAcceptHandler));
  server_ = std::make_unique<ServerImpl<Backend>>(&service_, 1234, callbacks_);

  // Call onAccept handler with non-error errorcode
  // Server should call onClientConnected callback and call async_accept again
  // Connection should call async_read
  ConnectionId connectionId;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  EXPECT_CALL(callbacksMock_, onClientConnected(_)).WillOnce(SaveArg<0>(&connectionId));
  EXPECT_CALL(service_, acceptor_async_accept(_, _));
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  onAcceptHandler(Backend::Error::no_error);

  // Call read handler with error (e.g. unexpected disconnect)
  // Server should call onClientDisconnected callback
  EXPECT_CALL(callbacksMock_, onClientDisconnected(connectionId));
  EXPECT_CALL(service_, socket_shutdown(_, _));
  EXPECT_CALL(service_, socket_close(_));
  readHandler(Backend::Error::other_error, 0);

  // Delete Server, should call cancel
  EXPECT_CALL(service_, acceptor_cancel());
  server_.reset();
}

TEST_F(ServerTest, OnPacketReceived)
{
  using ::testing::_;
  using ::testing::SaveArg;

  // Create Server, save onAccept handler
  std::function<void(const Backend::ErrorCode&)> onAcceptHandler;
  EXPECT_CALL(service_, acceptor_async_accept(_, _)).WillOnce(SaveArg<1>(&onAcceptHandler));
  server_ = std::make_unique<ServerImpl<Backend>>(&service_, 1234, callbacks_);

  // Call onAccept handler with non-error errorcode
  // Server should call onClientConnected callback and call async_accept again
  // Connection should call async_read
  ConnectionId connectionId;
  uint8_t* readBuffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  EXPECT_CALL(callbacksMock_, onClientConnected(_)).WillOnce(SaveArg<0>(&connectionId));
  EXPECT_CALL(service_, acceptor_async_accept(_, _));
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(DoAll(SaveArg<1>(&readBuffer), SaveArg<3>(&readHandler)));
  onAcceptHandler(Backend::Error::no_error);
  ASSERT_NE(nullptr, readBuffer);

  // Send packet header to Connection (packet data size = 1)
  readBuffer[0] = 0x01;
  readBuffer[1] = 0x00;
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(DoAll(SaveArg<1>(&readBuffer), SaveArg<3>(&readHandler)));
  readHandler(Backend::Error::no_error, 2);
  ASSERT_NE(nullptr, readBuffer);

  // Send packet data to Connection, Server should call onPacketReceived
  readBuffer[0] = 0x84;
  EXPECT_CALL(callbacksMock_, onPacketReceived(connectionId, _));
  EXPECT_CALL(service_, async_read(_, _, _, _));
  readHandler(Backend::Error::no_error, 1);

  // NOTE: We can not verify the IncomingPacket since the pointer is only valid during the function call
  // To be able to verify it we would need to create a custom Action and attach it to the EXPECT_CALL

  // Delete Server, should call cancel
  // And the Connection should call socket_shutdown and socket_close
  EXPECT_CALL(service_, socket_shutdown(_, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(service_, acceptor_cancel());
  server_.reset();
}
