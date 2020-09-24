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

#include "client_factory.h"

#include <memory>

#define ASIO_STANDALONE 1
#include <asio.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "websocketpp_client_backend.h"
#include "connection_impl.h"

namespace
{

// TODO(simon): not sure if we can use one single client for multiple connections
std::unique_ptr<websocketpp::client<websocketpp::config::asio_client>> websocketpp_client;

struct PendingConnection
{
  PendingConnection(std::unique_ptr<network::WebsocketClient>&& client,
                    network::ClientFactory::Callbacks callbacks)
      : client(std::move(client)),
        callbacks(std::move(callbacks))
  {
  }

  std::unique_ptr<network::WebsocketClient> client;
  network::ClientFactory::Callbacks callbacks;
};

std::vector<PendingConnection> pending_connections;

void handleOpen(websocketpp::connection_hdl hdl)
{
  LOG_DEBUG("%s", __func__);

  std::error_code ec;
  auto connection = websocketpp_client->get_con_from_hdl(std::move(hdl), ec);
  if (ec)
  {
    LOG_ERROR("%s: get_con_from_hdl failed: %s", __func__, ec.message().c_str());
    return;
  }

  auto it = std::find_if(pending_connections.begin(),
                         pending_connections.end(),
                         [&connection](const PendingConnection& pending_connection)
                         {
                           return pending_connection.client->getWebsocketppConnection() == connection;
                         });
  if (it == pending_connections.end())
  {
    LOG_ERROR("%s: could not find pending connection", __func__);
    return;
  }

  // Create and send ConnectionImpl to user
  auto socket = network::WebsocketBackend::Socket(std::move(it->client));
  it->callbacks.on_connected(std::make_unique<network::ConnectionImpl<network::WebsocketBackend>>(std::move(socket)));

  // Delete pending connection
  pending_connections.erase(it);
}

void handleFail(websocketpp::connection_hdl hdl)
{
  LOG_DEBUG("%s", __func__);

  std::error_code ec;
  auto connection = websocketpp_client->get_con_from_hdl(std::move(hdl), ec);
  if (ec)
  {
    LOG_ERROR("%s: get_con_from_hdl failed: %s", __func__, ec.message().c_str());
    return;
  }

  auto it = std::find_if(pending_connections.begin(),
                         pending_connections.end(),
                         [&connection](const PendingConnection& pending_connection)
                         {
                           return pending_connection.client->getWebsocketppConnection() == connection;
                         });
  if (it == pending_connections.end())
  {
    LOG_ERROR("%s: could not find pending connection", __func__);
    return;
  }

  // Call on_connect_failure callback and erase pending connection
  it->callbacks.on_connect_failure();
  pending_connections.erase(it);
}

}  // namespace

namespace network
{

bool ClientFactory::createWebsocketClient(asio::io_context* io_context, const std::string& uri, const Callbacks& callbacks)
{
  // Create and initialize websocketpp client if not already done
  if (!websocketpp_client)
  {
    websocketpp_client = std::make_unique<websocketpp::client<websocketpp::config::asio_client>>();
    websocketpp_client->init_asio(io_context);
    websocketpp_client->set_open_handler(&handleOpen);
    websocketpp_client->set_fail_handler(&handleFail);
  }

  // Create websocketpp connection
  std::error_code ec;
  auto websocketpp_connection = websocketpp_client->get_connection(uri, ec);
  if (ec)
  {
    LOG_INFO("%s: could not create websocketpp connection: %s", __func__, ec.message().c_str());
    return false;
  }

  // Create WebsocketClient
  auto websocket_client = std::make_unique<WebsocketClient>(websocketpp_connection);

  // Start connect procedure
  websocketpp_client->connect(websocketpp_connection);

  // Create and store PendingConnection
  pending_connections.emplace_back(std::move(websocket_client), callbacks);

  return true;
}

}  // namespace network
