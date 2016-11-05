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

#include "connection.h"
#include "backend_mock.h"

class ConnectionTest : public ::testing::Test
{
 public:
  ConnectionTest()
    : service_(),
      callbacksMock_(),
      callbacks_(
      {
        std::bind(&CallbacksMock::onPacketReceived, &callbacksMock_, std::placeholders::_1),
        std::bind(&CallbacksMock::onDisconnected, &callbacksMock_),
        std::bind(&CallbacksMock::onConnectionClosed, &callbacksMock_),
      }),
      connection_()  // Created in each test case
  {
  }

  struct CallbacksMock
  {
    MOCK_METHOD1(onPacketReceived, void(IncomingPacket*));
    MOCK_METHOD0(onDisconnected, void());
    MOCK_METHOD0(onConnectionClosed, void());
  };

 protected:
  // Helpers
  Backend::Service service_;
  CallbacksMock callbacksMock_;
  Connection<Backend>::Callbacks callbacks_;

  // Under test
  std::unique_ptr<Connection<Backend>> connection_;
};

TEST_F(ConnectionTest, ConstructAndDelete)
{
  using ::testing::_;

  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::unique_ptr<Connection<Backend>>(new Connection<Backend>(Backend::Socket(service_), callbacks_));

  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());
  connection_.reset();
}

TEST_F(ConnectionTest, Close)
{
  using ::testing::_;

  // Test with force == false
  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::unique_ptr<Connection<Backend>>(new Connection<Backend>(Backend::Socket(service_), callbacks_));

  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());
  connection_->close(false);

  // Test with force == true
  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::unique_ptr<Connection<Backend>>(new Connection<Backend>(Backend::Socket(service_), callbacks_));

  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());
  connection_->close(true);
}

TEST_F(ConnectionTest, SendPacket)
{
  using ::testing::_;
  using ::testing::SaveArg;

  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::unique_ptr<Connection<Backend>>(new Connection<Backend>(Backend::Socket(service_), callbacks_));

  // Create an OutgoingPacket
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);
  auto packetLength = outgoingPacket.getLength();

  // Connection should send packet header first (2 bytes)
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;
  EXPECT_CALL(service_, async_write(_, _, 2, _)).WillOnce(SaveArg<3>(&writeHandler));
  connection_->sendPacket(std::move(outgoingPacket));

  // Respond to Connection and verify that packet data is sent next
  EXPECT_CALL(service_, async_write(_, _, packetLength, _)).WillOnce(SaveArg<3>(&writeHandler));
  writeHandler(0, 2);  // no error = 0, bytes sent = 2

  // Respond to Connection that 4 bytes was sent
  writeHandler(0, packetLength);

  // Close the connection
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());
  connection_->close(true);
}

TEST_F(ConnectionTest, ReceivePacket)
{
  // TODO
}

TEST_F(ConnectionTest, Disconnect)
{
  // TODO
}
