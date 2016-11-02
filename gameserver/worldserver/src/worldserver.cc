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

#include <deque>
#include <functional>
#include <memory>
#include <boost/asio.hpp>  //NOLINT

#include "configparser.h"
#include "logger.h"
#include "account.h"
#include "server.h"
#include "server_factory.h"
#include "incomingpacket.h"
#include "outgoingpacket.h"
#include "gameengine.h"
#include "gameengineproxy.h"
#include "protocol.h"
#include "protocol_71.h"
#include "world.h"
#include "worldfactory.h"

static AccountReader accountReader;
static GameEngineProxy gameEngineProxy;
static std::unique_ptr<Server> server;
static std::unique_ptr<World> world;

static std::unordered_map<ConnectionId, std::unique_ptr<Protocol>> protocols;  // TODO(gurka): Maybe change to array?

void onClientConnected(ConnectionId connectionId)
{
  LOG_DEBUG("%s: ConnectionId: %d", __func__, connectionId);

  // Create and store Protocol for this Connection
  // TODO(gurka): Need a different solution if we want to support different protocol versions
  // (We need to parse the login packet before we create a specific Protocol implementation)
  auto protocol = std::unique_ptr<Protocol>(new Protocol71(&gameEngineProxy,
                                                           world.get(),
                                                           connectionId,
                                                           server.get(),
                                                           &accountReader));

  protocols.emplace(std::piecewise_construct,
                    std::forward_as_tuple(connectionId),
                    std::forward_as_tuple(std::move(protocol)));
}

void onClientDisconnected(ConnectionId connectionId)
{
  LOG_DEBUG("%s: ConnectionId: %d", __func__, connectionId);

  // Delete Protocol
  protocols.erase(connectionId);
}

void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("%s: ConnectionId: %d Length: %u", __func__, connectionId, packet->getLength());

  protocols.at(connectionId)->parsePacket(packet);
}

int main(int argc, char* argv[])
{
  // Read configuration
  auto config = ConfigParser::parseFile("data/worldserver.cfg");
  if (!config.parsedOk())
  {
    printf("Could not parse config file: %s\n", config.getErrorMessage().c_str());
    printf("Will continue with default values\n");
  }

  // Read [server] settings
  auto serverPort = config.getInteger("server", "port", 7172);

  // Read [world] settings
  auto loginMessage     = config.getString("world", "login_message", "Welcome to LoginServer!");
  auto accountsFilename = config.getString("world", "accounts_file", "data/accounts.xml");
  auto dataFilename     = config.getString("world", "data_file", "data/data.dat");
  auto itemsFilename    = config.getString("world", "item_file", "data/items.xml");
  auto worldFilename    = config.getString("world", "world_file", "data/world.xml");

  // Read [logger] settings
  auto logger_account     = config.getString("logger", "account", "ERROR");
  auto logger_network     = config.getString("logger", "network", "ERROR");
  auto logger_utils       = config.getString("logger", "utils", "ERROR");
  auto logger_world       = config.getString("logger", "world", "ERROR");
  auto logger_worldserver = config.getString("logger", "worldserver", "ERROR");

  auto levelStringToEnum = [](const std::string& level)
  {
    if (level == "INFO")
    {
      return Logger::Level::INFO;
    }
    else if (level == "DEBUG")
    {
      return Logger::Level::DEBUG;
    }
    else  // "ERROR" or anything else
    {
      return Logger::Level::ERROR;
    }
  };

  // Set logger settings
  Logger::setLevel(Logger::Module::ACCOUNT,     levelStringToEnum(logger_account));
  Logger::setLevel(Logger::Module::NETWORK,     levelStringToEnum(logger_network));
  Logger::setLevel(Logger::Module::UTILS,       levelStringToEnum(logger_utils));
  Logger::setLevel(Logger::Module::WORLD,       levelStringToEnum(logger_world));
  Logger::setLevel(Logger::Module::WORLDSERVER, levelStringToEnum(logger_worldserver));

  // Print configuration values
  printf("--------------------------------------------------------------------------------\n");
  printf("WorldServer configuration\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("Server port:               %d\n", serverPort);
  printf("\n");
  printf("Login message:             %s\n", loginMessage.c_str());
  printf("Accounts filename:         %s\n", accountsFilename.c_str());
  printf("Data filename:             %s\n", dataFilename.c_str());
  printf("Items filename:            %s\n", itemsFilename.c_str());
  printf("World filename:            %s\n", worldFilename.c_str());
  printf("\n");
  printf("Account logging:           %s\n", logger_account.c_str());
  printf("Network logging:           %s\n", logger_network.c_str());
  printf("Utils logging:             %s\n", logger_utils.c_str());
  printf("World logging:             %s\n", logger_world.c_str());
  printf("Worldserver logging:       %s\n", logger_worldserver.c_str());
  printf("--------------------------------------------------------------------------------\n");

  boost::asio::io_service io_service;

  // Create Server
  Server::Callbacks callbacks =
  {
    &onClientConnected,
    &onClientDisconnected,
    &onPacketReceived,
  };
  server = ServerFactory::createServer(&io_service, serverPort, callbacks);

  // Create World
  world = WorldFactory::createWorld(dataFilename, itemsFilename, worldFilename);
  if (!world)
  {
    LOG_ERROR("World could not be loaded");
    return -1;
  }

  // Create GameEngine
  auto gameEngine = std::unique_ptr<GameEngine>(new GameEngine(&io_service,
                                                               loginMessage,
                                                               world.get()));
  gameEngineProxy.setGameEngine(std::move(gameEngine));

  // Load AccountReader
  if (!accountReader.loadFile(accountsFilename))
  {
    LOG_ERROR("Could not load accounts file: %s", accountsFilename.c_str());
    return -2;
  }

  // Stop on ^C from user
  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
  signals.async_wait(std::bind(&boost::asio::io_service::stop, &io_service));

  // Start Server, GameEngine and io_service
  if (!server->start())
  {
    LOG_ERROR("Could not start Server");
    return -3;
  }

  if (!gameEngineProxy.start())
  {
    LOG_ERROR("Could not start GameEngine");
    return -4;
  }

  // run() will continue to run until ^C from user is catched
  io_service.run();

  LOG_INFO("Stopping GameEngine");
  gameEngineProxy.stop();

  LOG_INFO("Stopping Server");
  server->stop();

  return 0;
}
