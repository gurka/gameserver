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

#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <asio.hpp>

// utils
#include "config_parser.h"
#include "logger.h"

// network
#include "server_factory.h"
#include "server.h"
#include "connection.h"

// replayserver
#include "replay_connection.h"


// We need to use unique_ptr, so that we can deallocate everything before
// static things (like Logger) gets deallocated
static asio::io_context io_context;
static std::unique_ptr<network::Server> server;
static std::unique_ptr<network::Server> websocket_server;

using ConnectionId = int;
static std::unordered_map<ConnectionId, std::unique_ptr<ReplayConnection>> connections;

void onClientConnected(std::unique_ptr<network::Connection>&& connection)
{
  static ConnectionId next_connection_id = 0;

  const auto connection_id = next_connection_id;
  next_connection_id += 1;

  LOG_DEBUG("%s: connection_id: %d", __func__, connection_id);

  const auto on_close = [connection_id]()
  {
    LOG_DEBUG("onCloseProtocol: connection_id: %d", connection_id);
    connections.erase(connection_id);
  };

  auto replay_connection = std::make_unique<ReplayConnection>(&io_context, on_close, std::move(connection));
  connections.emplace(std::piecewise_construct,
                      std::forward_as_tuple(connection_id),
                      std::forward_as_tuple(std::move(replay_connection)));
}

int main()
{
  // Read configuration
  const auto config = utils::ConfigParser::parseFile("data/replayserver.cfg");
  if (!config.parsedOk())
  {
    printf("Could not parse config file: %s\n", config.getErrorMessage().c_str());
    printf("Will continue with default values\n");
  }

  // Read [server] settings
  const auto server_port = config.getInteger("server", "port", 7172);
  const auto ws_server_port = server_port + 1000;

  // Read [logger] settings
  const auto logger_network      = config.getString("logger", "network",      "ERROR");
  const auto logger_utils        = config.getString("logger", "utils",        "ERROR");
  const auto logger_replayserver = config.getString("logger", "replayserver", "ERROR");

  // Set logger settings
  utils::Logger::setLevel("network",      logger_network);
  utils::Logger::setLevel("utils",        logger_utils);
  utils::Logger::setLevel("replayserver", logger_replayserver);

  // Print configuration values
  printf("--------------------------------------------------------------------------------\n");
  printf("ReplayServer configuration\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("Server port:               %d\n", server_port);
  printf("Websocket server port:     %d\n", ws_server_port);
  printf("\n");
  printf("Network logging:           %s\n", logger_network.c_str());
  printf("Utils logging:             %s\n", logger_utils.c_str());
  printf("Replayserver logging:      %s\n", logger_replayserver.c_str());
  printf("--------------------------------------------------------------------------------\n");

  LOG_INFO("Starting ReplayServer!");

  // Create Server
  server = network::ServerFactory::createServer(&io_context, server_port, &onClientConnected);

  // Create websocket server
  websocket_server = network::ServerFactory::createWebsocketServer(&io_context, ws_server_port, &onClientConnected);

  LOG_INFO("ReplayServer started!");

  // run() will continue to run until ^C from user is catched
  asio::signal_set signals(io_context, SIGINT, SIGTERM);
  signals.async_wait([](const std::error_code& error, int signal_number)
  {
    LOG_INFO("%s: received error: %s, signal_number: %d, stopping io_context",
             __func__,
             error.message().c_str(),
             signal_number);
    io_context.stop();
  });
  io_context.run();

  LOG_INFO("Stopping ReplayServer!");

  // Deallocate things (in reverse order of construction)
  connections.clear();
  websocket_server.reset();
  server.reset();

  return 0;
}
