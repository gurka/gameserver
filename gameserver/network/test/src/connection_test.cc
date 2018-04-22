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

#include "connection.h"
#include "backend_mock.h"

// To be able to match IncomingPackets
bool operator==(const IncomingPacket& a, const IncomingPacket& b)
{
  if (a.getLength() != b.getLength())
  {
    return false;
  }

  const auto aBytes = a.peekBytes(a.getLength());
  const auto bBytes = b.peekBytes(b.getLength());

  for (auto i = 0u; i < aBytes.size(); i++)
  {
    if (aBytes[i] != bBytes[i])
    {
      return false;
    }
  }

  return true;
}

class ConnectionTest : public ::testing::Test
{
 public:
  ConnectionTest()
    : service_(),
      callbacksMock_(),
      callbacks_(),
      connection_()  // Created in each test case
  {
    callbacks_.onPacketReceived = [this](IncomingPacket* packet)
    {
      callbacksMock_.onPacketReceived(packet);
    };

    callbacks_.onDisconnected = [this]()
    {
      callbacksMock_.onDisconnected();
    };

    callbacks_.onConnectionClosed = [this]()
    {
      callbacksMock_.onConnectionClosed();
    };
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
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);
  connection_.reset();
}

TEST_F(ConnectionTest, Close)
{
  using ::testing::_;

  // Test with force = false
  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());
  connection_->close(false);

  // Test with force = true
  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

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
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  // Create an OutgoingPacket
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);

  // Connection should send packet header first (2 bytes)
  const std::uint8_t* buffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;
  EXPECT_CALL(service_, async_write(_, _, 2, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&writeHandler)));
  connection_->sendPacket(std::move(outgoingPacket));

  // Verify buffer content
  EXPECT_EQ(0x04, buffer[0]);
  EXPECT_EQ(0x00, buffer[1]);

  // Respond to Connection and verify that packet data is sent next
  EXPECT_CALL(service_, async_write(_, _, 4, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&writeHandler)));
  writeHandler(Backend::Error::no_error, 2);

  // Verify buffer content
  EXPECT_EQ(0x78, buffer[0]);
  EXPECT_EQ(0x56, buffer[1]);
  EXPECT_EQ(0x34, buffer[2]);
  EXPECT_EQ(0x12, buffer[3]);

  // Respond to Connection that 4 bytes was sent
  writeHandler(Backend::Error::no_error, 4);

  // Close the connection
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());
  connection_->close(true);
}

TEST_F(ConnectionTest, ReceivePacket)
{
  using ::testing::_;
  using ::testing::SaveArg;
  using ::testing::Pointee;

  // We need to capture the variables in the async_read call
  std::uint8_t* buffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  EXPECT_CALL(service_, async_read(_, _, 2, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&readHandler)));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  // Write packet header to buffer (packet data is 4 bytes)
  ASSERT_NE(nullptr, buffer);
  buffer[0] = 0x04;
  buffer[1] = 0x00;

  // When Connection receives packet header it should call async_read to receive packet data (4 bytes)
  EXPECT_CALL(service_, async_read(_, _, 4, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&readHandler)));
  readHandler(Backend::Error::no_error, 2);

  // Write packet data to buffer
  ASSERT_NE(nullptr, buffer);
  buffer[0] = 0x12;
  buffer[1] = 0x34;
  buffer[2] = 0x56;
  buffer[3] = 0x78;

  // When Connection receives packet data it should call onPacketReceived callback
  // and call async_read again to receive next packet header
  const std::uint8_t expectedPacketData[4] = { 0x12, 0x34, 0x56, 0x78 };
  IncomingPacket expectedPacket { expectedPacketData, 4u };
  EXPECT_CALL(callbacksMock_, onPacketReceived(Pointee(expectedPacket)));
  EXPECT_CALL(service_, async_read(_, _, 2, _));
  readHandler(Backend::Error::no_error, 4);

  // Close the connection
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());
  connection_->close(true);
}

TEST_F(ConnectionTest, Disconnect_1)
{
  // Disconnect scenario 1: during async_read of packet header
  using ::testing::_;
  using ::testing::SaveArg;

  // We need to capture the callback in the async_read call
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  EXPECT_CALL(service_, async_read(_, _, 2, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  // onDisconnect callback should be called when async_read handler is called with error
  EXPECT_CALL(callbacksMock_, onDisconnected());

  // And the Connection should be closed
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());

  readHandler(Backend::Error::other_error, 0);

  connection_.reset();  // Shouldn't have any side effects
}

TEST_F(ConnectionTest, Disconnect_2)
{
  // Disconnect scenario 2: during async_read of packet data
  using ::testing::_;
  using ::testing::SaveArg;

  // We need to capture the callback in the async_read call
  std::uint8_t* buffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  EXPECT_CALL(service_, async_read(_, _, 2, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&readHandler)));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  // Write packet header to buffer (packet data is 1 byte)
  ASSERT_NE(nullptr, buffer);
  buffer[0] = 0x01;
  buffer[1] = 0x00;

  // Call readHandler
  EXPECT_CALL(service_, async_read(_, _, 1, _)).WillOnce(SaveArg<3>(&readHandler));
  readHandler(Backend::Error::no_error, 2);

  // onDisconnect callback should be called when async_read handler is called with error
  EXPECT_CALL(callbacksMock_, onDisconnected());

  // And the Connection should be closed
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());

  readHandler(Backend::Error::other_error, 0);

  connection_.reset();  // Shouldn't have any side effects
}

TEST_F(ConnectionTest, Disconnect_3)
{
  // Disconnect scenario 3: during async_write of packet header
  using ::testing::_;
  using ::testing::SaveArg;

  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  // Create an OutgoingPacket
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);

  // Save writeHandler
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;
  EXPECT_CALL(service_, async_write(_, _, 2, _)).WillOnce(SaveArg<3>(&writeHandler));
  connection_->sendPacket(std::move(outgoingPacket));

  // Nothing should happen when there occurs an error while sending a packet
  writeHandler(Backend::Error::other_error, 0);

  connection_.reset();  // Shouldn't have any side effects
}

TEST_F(ConnectionTest, Disconnect_4)
{
  // Disconnect scenario 4: during async_write of packet data
  using ::testing::_;
  using ::testing::SaveArg;

  EXPECT_CALL(service_, async_read(_, _, _, _));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  // Create an OutgoingPacket
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);

  // Save writeHandler from call to async_write
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;
  EXPECT_CALL(service_, async_write(_, _, _, _)).WillOnce(SaveArg<3>(&writeHandler));
  connection_->sendPacket(std::move(outgoingPacket));

  // Call writeHandler, the Connection should no call async_write to send packet data
  EXPECT_CALL(service_, async_write(_, _, _, _)).WillOnce(SaveArg<3>(&writeHandler));
  writeHandler(Backend::Error::no_error, 2);

  // Nothing should happen when there occurs an error while sending a packet
  writeHandler(Backend::Error::other_error, 0);

  connection_.reset();  // Shouldn't have any side effects
}

TEST_F(ConnectionTest, Disconnect_5)
{
  // Disconnect scenario 5: both async_read and async_write fails
  using ::testing::_;
  using ::testing::SaveArg;

  // We need to capture the callback in the async_read call
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_ = std::make_unique<Connection<Backend>>(Backend::Socket(service_), callbacks_);

  // Create an OutgoingPacket
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);

  // Save writeHandler
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;
  EXPECT_CALL(service_, async_write(_, _, _, _)).WillOnce(SaveArg<3>(&writeHandler));
  connection_->sendPacket(std::move(outgoingPacket));

  // onDisconnect callback should be called ONCE when both async_write and async_read fails
  EXPECT_CALL(callbacksMock_, onDisconnected());

  // And the Connection should be closed (everything called only ONCE)
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onConnectionClosed());

  readHandler(Backend::Error::other_error, 0);
  writeHandler(Backend::Error::other_error, 0);

  connection_.reset();  // Shouldn't have any side effects
}
