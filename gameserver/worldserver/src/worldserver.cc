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
#include <boost/asio.hpp>

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
static std::unique_ptr<GameEngineQueue> game_engine_queue;
static std::unique_ptr<GameEngine> game_engine;
static std::unique_ptr<account::AccountReader> account_reader;
static std::unique_ptr<Server> server;

using ProtocolId = int;
static std::unordered_map<ProtocolId, std::unique_ptr<Protocol>> protocols;

void onClientConnected(std::unique_ptr<Connection>&& connection)
{
  static ProtocolId next_protocol_id = 0;

  const auto protocol_id = next_protocol_id;
  next_protocol_id += 1;

  LOG_DEBUG("%s: protocol_id: %d", __func__, protocol_id);

  // Create and store Protocol for this Connection
  auto protocol = std::make_unique<Protocol>([protocol_id]()
                                             {
                                               LOG_DEBUG("onCloseProtocol: protocol_id: %d", protocol_id);
                                               protocols.erase(protocol_id);
                                             },
                                             std::move(connection),
                                             game_engine->getWorldInterface(),
                                             game_engine_queue.get(),
                                             account_reader.get());

  protocols.emplace(std::piecewise_construct,
                    std::forward_as_tuple(protocol_id),
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
  const auto server_port = config.getInteger("server", "port", 7172);

  // Read [world] settings
  const auto login_message     = config.getString("world", "login_message", "Welcome to LoginServer!");
  const auto accounts_filename = config.getString("world", "accounts_file", "data/accounts.xml");
  const auto data_filename     = config.getString("world", "data_file",     "data/data.dat");
  const auto items_filename    = config.getString("world", "item_file",     "data/items.xml");
  const auto world_filename    = config.getString("world", "world_file",    "data/world.xml");

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
  printf("Server port:               %d\n", server_port);
  printf("\n");
  printf("Login message:             %s\n", login_message.c_str());
  printf("Accounts filename:         %s\n", accounts_filename.c_str());
  printf("Data filename:             %s\n", data_filename.c_str());
  printf("Items filename:            %s\n", items_filename.c_str());
  printf("World filename:            %s\n", world_filename.c_str());
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
  game_engine = std::make_unique<GameEngine>();
  game_engine_queue = std::make_unique<GameEngineQueue>(game_engine.get(), &io_context);

  // Initialize GameEngine
  if (!game_engine->init(game_engine_queue.get(), login_message, data_filename, items_filename, world_filename))
  {
    LOG_ERROR("Could not initialize GameEngine");
    return 1;
  }

  // Create and load AccountReader
  account_reader = std::make_unique<account::AccountReader>();
  if (!account_reader->loadFile(accounts_filename))
  {
    LOG_ERROR("Could not load accounts file: %s", accounts_filename.c_str());
    return 1;
  }

  // Create Server
  server = ServerFactory::createServer(&io_context, server_port, &onClientConnected);

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
  server.reset();
  account_reader.reset();
  game_engine.reset();
  game_engine_queue.reset();

  return 0;
}
