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

#include "connection_impl.h"
#include "backend_mock.h"

namespace network
{

using ::testing::_;
using ::testing::SaveArg;
using ::testing::Pointee;
using ::testing::Return;

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
    callbacks_.on_packet_received = [this](IncomingPacket* packet)
    {
      callbacksMock_.onPacketReceived(packet);
    };

    callbacks_.on_disconnected = [this]()
    {
      callbacksMock_.onDisconnected();
    };
  }

  struct CallbacksMock
  {
    MOCK_METHOD1(onPacketReceived, void(IncomingPacket*));
    MOCK_METHOD0(onDisconnected, void());
  };

 protected:
  // Helpers
  Backend::Service service_;
  CallbacksMock callbacksMock_;
  Connection::Callbacks callbacks_;

  // Under test
  std::unique_ptr<ConnectionImpl<Backend>> connection_;
};

TEST_F(ConnectionTest, ConstructAndDelete)
{
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  connection_.reset();
}

TEST_F(ConnectionTest, InitClose)
{
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;

  // Since there is no send in progress, close(false) and close(true) will behave
  // the same way, but still test them both here

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));

  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_->init(callbacks_);

  // Close the connection with force = false
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  connection_->close(false);

  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(false));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));

  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_->init(callbacks_);

  // Close the connection with force = true
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  connection_->close(true);

  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(false));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();
}

TEST_F(ConnectionTest, ReceivePacket)
{
  std::uint8_t* buffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  EXPECT_CALL(service_, async_read(_, _, 2, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&readHandler)));
  connection_->init(callbacks_);
  ASSERT_NE(nullptr, buffer);

  // Send packet header to connection (packet data is 4 bytes)
  buffer[0] = 0x04;
  buffer[1] = 0x00;
  buffer = nullptr;
  EXPECT_CALL(service_, async_read(_, _, 4, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&readHandler)));
  readHandler(Backend::Error::no_error, 2);
  ASSERT_NE(nullptr, buffer);

  // Send packet data to connection
  buffer[0] = 0x12;
  buffer[1] = 0x34;
  buffer[2] = 0x56;
  buffer[3] = 0x78;
  const std::uint8_t expectedPacketData[] = { 0x12, 0x34, 0x56, 0x78 };
  IncomingPacket expectedPacket { expectedPacketData, 4u };
  EXPECT_CALL(callbacksMock_, onPacketReceived(Pointee(expectedPacket)));
  EXPECT_CALL(service_, async_read(_, _, 2, _));
  readHandler(Backend::Error::no_error, 4);

  // Close the connection
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  connection_->close(false);

  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(false));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();
}

TEST_F(ConnectionTest, SendPacket)
{
  const std::uint8_t* buffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_->init(callbacks_);

  // Create an OutgoingPacket
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);

  // Connection should send packet header first (2 bytes)
  EXPECT_CALL(service_, async_write(_, _, 2, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&writeHandler)));
  connection_->sendPacket(std::move(outgoingPacket));
  ASSERT_NE(nullptr, buffer);
  EXPECT_EQ(0x04, buffer[0]);
  EXPECT_EQ(0x00, buffer[1]);

  // Respond to Connection and verify that packet data is sent next
  buffer = nullptr;
  EXPECT_CALL(service_, async_write(_, _, 4, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&writeHandler)));
  writeHandler(Backend::Error::no_error, 2);
  ASSERT_NE(nullptr, buffer);
  EXPECT_EQ(0x78, buffer[0]);
  EXPECT_EQ(0x56, buffer[1]);
  EXPECT_EQ(0x34, buffer[2]);
  EXPECT_EQ(0x12, buffer[3]);

  // Respond to Connection that 4 bytes was sent
  writeHandler(Backend::Error::no_error, 4);

  // Close the connection
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  connection_->close(false);

  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(false));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();
}

TEST_F(ConnectionTest, DisconnectInHeaderReadCall)
{
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_->init(callbacks_);

  // As there is no send in progress the connection should close the socket
  // and call the onDisconnected callback directly when the read call fails
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();
}

TEST_F(ConnectionTest, DisconnectInDataReadCall)
{
  std::uint8_t* buffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  EXPECT_CALL(service_, async_read(_, _, 2, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&readHandler)));
  connection_->init(callbacks_);
  ASSERT_NE(nullptr, buffer);

  // Set packet length to 100
  buffer[0] = 0x64;
  buffer[1] = 0x00;
  EXPECT_CALL(service_, async_read(_, _, 0x64, _)).WillOnce(SaveArg<3>(&readHandler));
  readHandler(Backend::no_error, 2);

  // As there is no send in progress the connection should close the socket
  // and call the onDisconnected callback directly when the read call fails
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();
}

TEST_F(ConnectionTest, DisconnectInHeaderWriteCall)
{
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_->init(callbacks_);

  // Send a packet (header will be sent first)
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);
  EXPECT_CALL(service_, async_write(_, _, 2, _)).WillOnce(SaveArg<3>(&writeHandler));
  connection_->sendPacket(std::move(outgoingPacket));

  // Have the write call fail, the socket should be closed but onDisconnected
  // should not be called yet since a read call is ongoing
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  writeHandler(Backend::other_error, 0);

  // onDisconnected should be called when the read call fails
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(false));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();
}

TEST_F(ConnectionTest, DisconnectInDataWriteCall)
{
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;
  std::function<void(const Backend::ErrorCode&, std::size_t)> writeHandler;

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  EXPECT_CALL(service_, async_read(_, _, _, _)).WillOnce(SaveArg<3>(&readHandler));
  connection_->init(callbacks_);

  // Send a packet (header will be sent first)
  OutgoingPacket outgoingPacket;
  outgoingPacket.addU32(0x12345678);
  EXPECT_CALL(service_, async_write(_, _, 2, _)).WillOnce(SaveArg<3>(&writeHandler));
  connection_->sendPacket(std::move(outgoingPacket));

  // Header sent OK
  EXPECT_CALL(service_, async_write(_, _, 4, _)).WillOnce(SaveArg<3>(&writeHandler));
  writeHandler(Backend::no_error, 2);

  // Have the write call fail, the socket should be closed but onDisconnected
  // should not be called yet since a read call is ongoing
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  writeHandler(Backend::other_error, 0);

  // onDisconnected should be called when the read call fails
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(false));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::operation_aborted, 0);
  connection_.reset();
}

TEST_F(ConnectionTest, ReadsPacketLengthZero)
{
  std::uint8_t* buffer = nullptr;
  std::function<void(const Backend::ErrorCode&, std::size_t)> readHandler;

  // Create and initialize connection
  connection_ = std::make_unique<ConnectionImpl<Backend>>(Backend::Socket(service_));
  EXPECT_CALL(service_, async_read(_, _, 2, _)).WillOnce(DoAll(SaveArg<1>(&buffer), SaveArg<3>(&readHandler)));
  connection_->init(callbacks_);
  ASSERT_NE(nullptr, buffer);

  // Send packet header with packet length zero to connection
  // As there is no send in progress the connection should close the socket
  // and call the onDisconnected callback directly when the invalid packet length is read
  buffer[0] = 0x00;
  buffer[1] = 0x00;
  EXPECT_CALL(service_, socket_is_open()).WillOnce(Return(true));
  EXPECT_CALL(service_, socket_shutdown(Backend::shutdown_both, _));
  EXPECT_CALL(service_, socket_close(_));
  EXPECT_CALL(callbacksMock_, onDisconnected());
  readHandler(Backend::Error::no_error, 2);

  connection_.reset();
}

}  // namespace network

// TODO(simon): tests to do:
//
// close(true) in receivePacket
// close(false) in receivePacket
// close(true) while reading
// close(true) while reading and writing
// close(false) while reading
// close(false) while reading and writing
// readHandler fails before writeHandler
