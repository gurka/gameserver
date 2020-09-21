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
#include "network.h"

#include <cstdint>
#include <string>
#include <vector>

#define ASIO_STANDALONE 1
#include <asio.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "logger.h"
#include "outgoing_packet.h"
#include "incoming_packet.h"

namespace wsclient::network
{

using WebsocketClient = websocketpp::client<websocketpp::config::asio_client>;

static std::unique_ptr<WebsocketClient> client;
static WebsocketClient::connection_ptr connection;
static std::function<void(IncomingPacket*)> callback;
static std::vector<std::uint8_t> read_buffer;

void start(asio::io_context* io_context, const std::string& uri, const std::function<void(IncomingPacket*)> callback_in)
{
  (void)uri;

  if (client)
  {
    LOG_ERROR("%s: client already exist", __func__);
    return;
  }

  callback = callback_in;

  client = std::make_unique<WebsocketClient>();
  client->init_asio(io_context);

  std::error_code ec;
  connection = client->get_connection(uri, ec);
  if (ec)
  {
    LOG_ERROR("%s: get_connection -> ec: %s", __func__, ec.message().c_str());
    return;
  }

  connection->set_open_handler([](websocketpp::connection_hdl)
  {
    LOG_INFO("%s: open", __func__);
  });

  connection->set_fail_handler([](websocketpp::connection_hdl)
  {
    LOG_INFO("%s: fail", __func__);
    connection.reset();
    client.reset();
  });

  connection->set_close_handler([](websocketpp::connection_hdl)
  {
    connection.reset();
    client.reset();
  });

  connection->set_message_handler([](websocketpp::connection_hdl, WebsocketClient::message_ptr msg)
  {
    const auto& payload = msg->get_payload();
    std::copy(reinterpret_cast<const std::uint8_t*>(payload.data()),
              reinterpret_cast<const std::uint8_t*>(payload.data() + payload.size()),
              std::back_inserter(read_buffer));

    while (read_buffer.size() >= 2)
    {
      const auto next_packet_length = read_buffer[0] | (read_buffer[1] << 8);
      if (static_cast<int>(read_buffer.size()) < next_packet_length + 2)
      {
        break;
      }

      IncomingPacket packet(&read_buffer[2], next_packet_length);
      callback(&packet);

      read_buffer.erase(read_buffer.begin(), read_buffer.begin() + 2 + next_packet_length);
    }
  });

  client->connect(connection);
}

void stop()
{
  if (connection)
  {
    connection->close(0, "");
  }
}

void sendPacket(OutgoingPacket&& packet)
{
  (void)packet;
}

}  // namespace wsclient::network
