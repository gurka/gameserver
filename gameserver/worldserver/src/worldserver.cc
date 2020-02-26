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

// account
#include "account.h"

// network
#include "server_factory.h"
#include "server.h"
#include "connection.h"

// gameengine
#include "game_engine.h"
#include "game_engine_queue.h"

// worldserver
#include "connection_ctrl.h"


// We need to use unique_ptr, so that we can deallocate everything before
// static things (like Logger) gets deallocated
static std::unique_ptr<gameengine::GameEngineQueue> game_engine_queue;
static std::unique_ptr<gameengine::GameEngine> game_engine;
static std::unique_ptr<account::AccountReader> account_reader;
static std::unique_ptr<network::Server> server;
static std::unique_ptr<network::Server> websocket_server;

using ConnectionId = int;
static std::unordered_map<ConnectionId, std::unique_ptr<ConnectionCtrl>> connections;

void onClientConnected(std::unique_ptr<network::Connection>&& connection)
{
  static ConnectionId next_connection_id = 0;

  const auto connection_id = next_connection_id;
  next_connection_id += 1;

  LOG_DEBUG("%s: connection_id: %d", __func__, connection_id);

  // Create and store Protocol for this Connection
  const auto on_close = [connection_id]()
  {
    LOG_DEBUG("onCloseProtocol: protocol_id: %d", connection_id);
    connections.erase(connection_id);
  };
  auto connection_ctrl = std::make_unique<ConnectionCtrl>(on_close,
                                                          std::move(connection),
                                                          game_engine->getWorld(),
                                                          game_engine_queue.get(),
                                                          account_reader.get());

  connections.emplace(std::piecewise_construct,
                      std::forward_as_tuple(connection_id),
                      std::forward_as_tuple(std::move(connection_ctrl)));
}

int main()
{
  // Read configuration
  const auto config = utils::ConfigParser::parseFile("data/worldserver.cfg");
  if (!config.parsedOk())
  {
    printf("Could not parse config file: %s\n", config.getErrorMessage().c_str());
    printf("Will continue with default values\n");
  }

  // Read [server] settings
  const auto server_port = config.getInteger("server", "port", 7172);
  const auto ws_server_port = server_port + 1000;

  // Read [world] settings
  const auto login_message     = config.getString("world", "login_message", "Welcome to LoginServer!");
  const auto accounts_filename = config.getString("world", "accounts_file", "data/accounts.xml");
  const auto data_filename     = config.getString("world", "data_file",     "data/data.dat");
  const auto items_filename    = config.getString("world", "item_file",     "data/items.xml");
  const auto world_filename    = config.getString("world", "world_file",    "data/world.xml");

  // Read [logger] settings
  const auto logger_account     = config.getString("logger", "account",     "ERROR");
  const auto logger_io          = config.getString("logger", "io",          "ERROR");
  const auto logger_network     = config.getString("logger", "network",     "ERROR");
  const auto logger_protocol    = config.getString("logger", "protocol",    "ERROR");
  const auto logger_utils       = config.getString("logger", "utils",       "ERROR");
  const auto logger_world       = config.getString("logger", "world",       "ERROR");
  const auto logger_worldserver = config.getString("logger", "worldserver", "ERROR");

  // Set logger settings
  utils::Logger::setLevel("account",     logger_account);
  utils::Logger::setLevel("io",          logger_io);
  utils::Logger::setLevel("network",     logger_network);
  utils::Logger::setLevel("protocol",    logger_protocol);
  utils::Logger::setLevel("utils",       logger_utils);
  utils::Logger::setLevel("world",       logger_world);
  utils::Logger::setLevel("worldserver", logger_worldserver);

  // Print configuration values
  printf("--------------------------------------------------------------------------------\n");
  printf("WorldServer configuration\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("Server port:               %d\n", server_port);
  printf("Websocket server port:     %d\n", ws_server_port);
  printf("\n");
  printf("Login message:             %s\n", login_message.c_str());
  printf("Accounts filename:         %s\n", accounts_filename.c_str());
  printf("Data filename:             %s\n", data_filename.c_str());
  printf("Items filename:            %s\n", items_filename.c_str());
  printf("World filename:            %s\n", world_filename.c_str());
  printf("\n");
  printf("Account logging:           %s\n", logger_account.c_str());
  printf("IO logging:                %s\n", logger_io.c_str());
  printf("Network logging:           %s\n", logger_network.c_str());
  printf("Protocol logging:          %s\n", logger_protocol.c_str());
  printf("Utils logging:             %s\n", logger_utils.c_str());
  printf("World logging:             %s\n", logger_world.c_str());
  printf("Worldserver logging:       %s\n", logger_worldserver.c_str());
  printf("--------------------------------------------------------------------------------\n");

  LOG_INFO("Starting WorldServer!");

  asio::io_context io_context;

  // Create GameEngine and GameEngineQueue
  game_engine = std::make_unique<gameengine::GameEngine>();
  game_engine_queue = std::make_unique<gameengine::GameEngineQueue>(game_engine.get(), &io_context);

  // Initialize GameEngine
  if (!game_engine->init(game_engine_queue.get(), login_message, data_filename, items_filename, world_filename))
  {
    LOG_ERROR("Could not initialize GameEngine");
    game_engine.reset();
    game_engine_queue.reset();
    return 1;
  }

  // Create and load AccountReader
  account_reader = std::make_unique<account::AccountReader>();
  if (!account_reader->load(accounts_filename))
  {
    LOG_ERROR("Could not load accounts file: %s", accounts_filename.c_str());
    account_reader.reset();
    game_engine.reset();
    game_engine_queue.reset();
    return 1;
  }

  // Create Server
  server = network::ServerFactory::createServer(&io_context, server_port, &onClientConnected);

  // Create websocket server
  websocket_server = network::ServerFactory::createWebsocketServer(&io_context, ws_server_port, &onClientConnected);

  LOG_INFO("WorldServer started!");

  // run() will continue to run until ^C from user is catched
  asio::signal_set signals(io_context, SIGINT, SIGTERM);
  signals.async_wait([&io_context](const std::error_code& error, int signal_number)
  {
    LOG_INFO("%s: received error: %s, signal_number: %d, stopping io_context",
             __func__,
             error.message().c_str(),
             signal_number);
    io_context.stop();
  });
  io_context.run();

  LOG_INFO("Stopping WorldServer!");

  // Deallocate things (in reverse order of construction)
  connections.clear();
  websocket_server.reset();
  server.reset();
  account_reader.reset();
  game_engine.reset();
  game_engine_queue.reset();

  return 0;
}
