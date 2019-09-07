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

#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>  //NOLINT

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
#include "protocol.h"


// We need to use unique_ptr, so that we can deallocate everything before
// static things (like Logger) gets deallocated
static std::unique_ptr<GameEngineQueue> gameEngineQueue;
static std::unique_ptr<GameEngine> gameEngine;
static std::unique_ptr<AccountReader> accountReader;
static std::unique_ptr<Server> server;
static std::unique_ptr<Server> websocketServer;

using ProtocolId = int;
static std::unordered_map<ProtocolId, std::unique_ptr<Protocol>> protocols;

void onClientConnected(std::unique_ptr<Connection>&& connection)
{
  static ProtocolId nextProtocolId = 0;

  const auto protocolId = nextProtocolId;
  nextProtocolId += 1;

  LOG_DEBUG("%s: protocolId: %d", __func__, protocolId);

  // Create and store Protocol for this Connection
  auto protocol = std::make_unique<Protocol>([protocolId]()
                                             {
                                               LOG_DEBUG("onCloseProtocol: protocolId: %d", protocolId);
                                               protocols.erase(protocolId);
                                             },
                                             std::move(connection),
                                             gameEngineQueue.get(),
                                             accountReader.get());

  protocols.emplace(std::piecewise_construct,
                    std::forward_as_tuple(protocolId),
                    std::forward_as_tuple(std::move(protocol)));
}

int main()
{
  // Read configuration
  const auto config = ConfigParser::parseFile("data/worldserver.cfg");
  if (!config.parsedOk())
  {
    printf("Could not parse config file: %s\n", config.getErrorMessage().c_str());
    printf("Will continue with default values\n");
  }

  // Read [server] settings
  const auto serverPort = config.getInteger("server", "port", 7172);
  const auto wsServerPort = serverPort + 1000;

  // Read [world] settings
  const auto loginMessage     = config.getString("world", "login_message", "Welcome to LoginServer!");
  const auto accountsFilename = config.getString("world", "accounts_file", "data/accounts.xml");
  const auto dataFilename     = config.getString("world", "data_file", "data/data.dat");
  const auto itemsFilename    = config.getString("world", "item_file", "data/items.xml");
  const auto worldFilename    = config.getString("world", "world_file", "data/world.xml");

  // Read [logger] settings
  const auto logger_account     = config.getString("logger", "account", "ERROR");
  const auto logger_network     = config.getString("logger", "network", "ERROR");
  const auto logger_protocol    = config.getString("logger", "protocol", "ERROR");
  const auto logger_utils       = config.getString("logger", "utils", "ERROR");
  const auto logger_world       = config.getString("logger", "world", "ERROR");
  const auto logger_worldserver = config.getString("logger", "worldserver", "ERROR");

  // Set logger settings
  Logger::setLevel(Logger::Module::ACCOUNT,     logger_account);
  Logger::setLevel(Logger::Module::NETWORK,     logger_network);
  Logger::setLevel(Logger::Module::PROTOCOL,    logger_protocol);
  Logger::setLevel(Logger::Module::UTILS,       logger_utils);
  Logger::setLevel(Logger::Module::WORLD,       logger_world);
  Logger::setLevel(Logger::Module::WORLDSERVER, logger_worldserver);

  // Print configuration values
  printf("--------------------------------------------------------------------------------\n");
  printf("WorldServer configuration\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("Server port:               %d\n", serverPort);
  printf("Websocket server port:     %d\n", wsServerPort);
  printf("\n");
  printf("Login message:             %s\n", loginMessage.c_str());
  printf("Accounts filename:         %s\n", accountsFilename.c_str());
  printf("Data filename:             %s\n", dataFilename.c_str());
  printf("Items filename:            %s\n", itemsFilename.c_str());
  printf("World filename:            %s\n", worldFilename.c_str());
  printf("\n");
  printf("Account logging:           %s\n", logger_account.c_str());
  printf("Network logging:           %s\n", logger_network.c_str());
  printf("Protocol logging:          %s\n", logger_protocol.c_str());
  printf("Utils logging:             %s\n", logger_utils.c_str());
  printf("World logging:             %s\n", logger_world.c_str());
  printf("Worldserver logging:       %s\n", logger_worldserver.c_str());
  printf("--------------------------------------------------------------------------------\n");

  LOG_INFO("Starting WorldServer!");

  boost::asio::io_context io_context;

  // Create GameEngine and GameEngineQueue
  gameEngine = std::make_unique<GameEngine>();
  gameEngineQueue = std::make_unique<GameEngineQueue>(gameEngine.get(), &io_context);

  // Initialize GameEngine
  if (!gameEngine->init(gameEngineQueue.get(), loginMessage, dataFilename, itemsFilename, worldFilename))
  {
    LOG_ERROR("Could not initialize GameEngine");
    return 1;
  }

  // Create and load AccountReader
  accountReader = std::make_unique<AccountReader>();
  if (!accountReader->loadFile(accountsFilename))
  {
    LOG_ERROR("Could not load accounts file: %s", accountsFilename.c_str());
    return 1;
  }

  // Create Server
  server = ServerFactory::createServer(&io_context, serverPort, &onClientConnected);

  // Create websocket server
  websocketServer = ServerFactory::createWebsocketServer(&io_context, wsServerPort, &onClientConnected);

  LOG_INFO("WorldServer started!");

  // run() will continue to run until ^C from user is catched
  boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
  signals.async_wait([&io_context](const boost::system::error_code& error, int signal_number)
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
  protocols.clear();
  websocketServer.reset();
  server.reset();
  accountReader.reset();
  gameEngine.reset();
  gameEngineQueue.reset();

  return 0;
}
