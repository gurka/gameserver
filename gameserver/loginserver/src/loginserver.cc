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
#include "incoming_packet.h"
#include "outgoing_packet.h"

// We need to use unique_ptr, so that we can deallocate everything before
// static things (like Logger) gets deallocated
static std::unique_ptr<AccountReader> accountReader;
static std::unique_ptr<Server> server;

// Due to "Static/global string variables are not permitted."
static struct
{
  std::string motd;
} motd;

using ConnectionId = int;
static std::unordered_map<ConnectionId, std::unique_ptr<Connection>> connections;

void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("Parsing packet from connection id: %d", connectionId);

  while (!packet->isEmpty())
  {
    const auto packetId = packet->getU8();
    switch (packetId)
    {
      case 0x01:
      {
        LOG_DEBUG("Parsing login packet from connection id: %d", connectionId);

        const auto clientOs = packet->getU16();       // Client OS
        const auto clientVersion = packet->getU16();  // Client version
        packet->getBytes(12);                         // Client OS info
        const auto accountNumber = packet->getU32();
        std::string password = packet->getString();

        LOG_DEBUG("Client OS: %d Client version: %d Account number: %d Password: %s",
                  clientOs,
                  clientVersion,
                  accountNumber,
                  password.c_str());

        // Send outgoing packet
        OutgoingPacket response;

          // Add MOTD
        response.addU8(0x14);  // MOTD
        response.addString("0\n" + motd.motd);

        // Check if account exists
        if (!accountReader->accountExists(accountNumber))
        {
          LOG_DEBUG("%s: Account (%d) not found", __func__, accountNumber);
          response.addU8(0x0A);
          response.addString("Invalid account number");
        }
        // Check if password is correct
        else if (!accountReader->verifyPassword(accountNumber, password))
        {
          LOG_DEBUG("%s: Invalid password (%s) for account (%d)", __func__, password.c_str(), accountNumber);
          response.addU8(0x0A);
          response.addString("Invalid password");
        }
        else
        {
          const auto* account = accountReader->getAccount(accountNumber);
          LOG_DEBUG("%s: Account number (%d) and password (%s) OK", __func__, accountNumber, password.c_str());
          response.addU8(0x64);
          response.addU8(account->characters.size());
          for (const auto& character : account->characters)
          {
            response.addString(character.name);
            response.addString(character.worldName);
            response.addU32(character.worldIp);
            response.addU16(character.worldPort);
          }
          response.addU16(account->premiumDays);
        }

        LOG_DEBUG("Sending login response to connection_id: %d", connectionId);
        connections.at(connectionId)->sendPacket(std::move(response));

        LOG_DEBUG("Closing connection id: %d", connectionId);
        connections.at(connectionId)->close(false);
      }
      break;

      default:
      {
        LOG_DEBUG("Unknown packet from connection id: %d, packet id: %d", connectionId, packetId);
        connections.at(connectionId)->close(true);
      }
      break;
    }
  }
}

void onClientConnected(std::unique_ptr<Connection> connection)
{
  static ConnectionId nextConnectionId = 0;

  const auto connectionId = nextConnectionId;
  nextConnectionId += 1;

  LOG_DEBUG("%s: connectionId: %d", __func__, connectionId);

  connections.emplace(std::piecewise_construct,
                      std::forward_as_tuple(connectionId),
                      std::forward_as_tuple(std::move(connection)));

  Connection::Callbacks callbacks
  {
    // onPacketReceived
    [connectionId](IncomingPacket* packet)
    {
      LOG_DEBUG("onPacketReceived: connectionId: %d", connectionId);
      onPacketReceived(connectionId, packet);
    },

    // onDisconnected
    [connectionId]()
    {
      LOG_DEBUG("onDisconnected: connectionId: %d", connectionId);
      connections.erase(connectionId);
    }
  };
  connections.at(connectionId)->init(callbacks);
}

int main()
{
  // Read configuration
  auto config = ConfigParser::parseFile("data/loginserver.cfg");
  if (!config.parsedOk())
  {
    LOG_INFO("Could not parse config file: %s", config.getErrorMessage().c_str());
    LOG_INFO("Will continue with default values");
  }

  // Read [server] settings
  auto serverPort = config.getInteger("server", "port", 7171);

  // Read [login] settings
  motd.motd = config.getString("login", "motd", "Welcome to LoginServer!");
  auto accountsFilename = config.getString("login", "accounts_file", "data/accounts.xml");

  // Read [logger] settings
  auto logger_account     = config.getString("logger", "account", "ERROR");
  auto logger_loginserver = config.getString("logger", "loginserver", "ERROR");
  auto logger_network     = config.getString("logger", "network", "ERROR");
  auto logger_utils       = config.getString("logger", "utils", "ERROR");

  // Set logger settings
  Logger::setLevel(Logger::Module::ACCOUNT,     logger_account);
  Logger::setLevel(Logger::Module::LOGINSERVER, logger_loginserver);
  Logger::setLevel(Logger::Module::NETWORK,     logger_network);
  Logger::setLevel(Logger::Module::UTILS,       logger_utils);

  // Print configuration values
  printf("--------------------------------------------------------------------------------\n");
  printf("LoginServer configuration\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("Server port:               %d\n", serverPort);
  printf("\n");
  printf("Accounts filename:         %s\n", accountsFilename.c_str());
  printf("Message of the day:        %s\n", motd.motd.c_str());
  printf("\n");
  printf("Account logging:           %s\n", logger_account.c_str());
  printf("Loginserver logging:       %s\n", logger_loginserver.c_str());
  printf("Network logging:           %s\n", logger_network.c_str());
  printf("Utils logging:             %s\n", logger_utils.c_str());
  printf("--------------------------------------------------------------------------------\n");

  // Create io_context
  boost::asio::io_context io_context;

  // Create and load AccountReader
  accountReader = std::make_unique<AccountReader>();
  if (!accountReader->loadFile(accountsFilename))
  {
    LOG_ERROR("Could not load accounts file: %s", accountsFilename.c_str());
    return 1;
  }

  // Create Server
  server = ServerFactory::createServer(&io_context, serverPort, &onClientConnected);

  LOG_INFO("LoginServer started!");

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

  LOG_INFO("Stopping LoginServer!");

  // Deallocate things
  server.reset();
  accountReader.reset();

  return 0;
}
