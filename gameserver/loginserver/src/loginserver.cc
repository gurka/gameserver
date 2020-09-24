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
#include "incoming_packet.h"
#include "outgoing_packet.h"

// We need to use unique_ptr, so that we can deallocate everything before
// static things (like Logger) gets deallocated
static std::unique_ptr<account::AccountReader> account_reader;
static std::unique_ptr<network::Server> server;
static std::unique_ptr<network::Server> websocket_server;

// Due to "Static/global string variables are not permitted."
static struct
{
  std::string motd;
} motd;

using ConnectionId = int;
static std::unordered_map<ConnectionId, std::unique_ptr<network::Connection>> connections;

void onPacketReceived(ConnectionId connection_id, network::IncomingPacket* packet)
{
  LOG_DEBUG("Parsing packet from connection id: %d", connection_id);

  while (!packet->isEmpty())
  {
    const auto packet_id = packet->getU8();
    switch (packet_id)
    {
      case 0x01:
      {
        LOG_DEBUG("Parsing login packet from connection id: %d", connection_id);

        const auto client_os = packet->getU16();       // Client OS
        const auto client_version = packet->getU16();  // Client version
        packet->getBytes(12);                         // Client OS info
        const auto account_number = packet->getU32();
        std::string password = packet->getString();

        LOG_DEBUG("Client OS: %d Client version: %d Account number: %d Password: %s",
                  client_os,
                  client_version,
                  account_number,
                  password.c_str());

        // Send outgoing packet
        network::OutgoingPacket response;

          // Add MOTD
        response.addU8(0x14);  // MOTD
        response.addString("0\n" + motd.motd);

        // Check if account exists
        if (!account_reader->accountExists(account_number))
        {
          LOG_DEBUG("%s: Account (%d) not found", __func__, account_number);
          response.addU8(0x0A);
          response.addString("Invalid account number");
        }
        // Check if password is correct
        else if (!account_reader->verifyPassword(account_number, password))
        {
          LOG_DEBUG("%s: Invalid password (%s) for account (%d)", __func__, password.c_str(), account_number);
          response.addU8(0x0A);
          response.addString("Invalid password");
        }
        else
        {
          const auto* account = account_reader->getAccount(account_number);
          LOG_DEBUG("%s: Account number (%d) and password (%s) OK", __func__, account_number, password.c_str());
          response.addU8(0x64);
          response.addU8(account->characters.size());
          for (const auto& character : account->characters)
          {
            response.addString(character.name);
            response.addString(character.world_name);
            response.addU32(character.world_ip);
            response.addU16(character.world_port);
          }
          response.addU16(account->premium_days);
        }

        LOG_DEBUG("Sending login response to connection_id: %d", connection_id);
        connections.at(connection_id)->sendPacket(std::move(response));

        LOG_DEBUG("Closing connection id: %d", connection_id);
        connections.at(connection_id)->close(false);
      }
      break;

      default:
      {
        LOG_DEBUG("Unknown packet from connection id: %d, packet id: %d", connection_id, packet_id);
        connections.at(connection_id)->close(true);
      }
      break;
    }
  }
}

void onClientConnected(std::unique_ptr<network::Connection> connection)
{
  static ConnectionId next_connection_id = 0;

  const auto connection_id = next_connection_id;
  next_connection_id += 1;

  LOG_DEBUG("%s: connection_id: %d", __func__, connection_id);

  connections.emplace(std::piecewise_construct,
                      std::forward_as_tuple(connection_id),
                      std::forward_as_tuple(std::move(connection)));

  network::Connection::Callbacks callbacks
  {
    // onPacketReceived
    [connection_id](network::IncomingPacket* packet)
    {
      LOG_DEBUG("onPacketReceived: connection_id: %d", connection_id);
      onPacketReceived(connection_id, packet);
    },

    // onDisconnected
    [connection_id]()
    {
      LOG_DEBUG("onDisconnected: connection_id: %d", connection_id);
      connections.erase(connection_id);
    }
  };
  connections.at(connection_id)->init(callbacks, false);
}

int main()
{
  // Read configuration
  auto config = utils::ConfigParser::parseFile("data/loginserver.cfg");
  if (!config.parsedOk())
  {
    LOG_INFO("Could not parse config file: %s", config.getErrorMessage().c_str());
    LOG_INFO("Will continue with default values");
  }

  // Read [server] settings
  const auto server_port = config.getInteger("server", "port", 7171);
  const auto ws_server_port = server_port + 1000;

  // Read [login] settings
  motd.motd = config.getString("login", "motd", "Welcome to LoginServer!");
  auto accounts_filename = config.getString("login", "accounts_file", "data/accounts.xml");

  // Read [logger] settings
  auto logger_account     = config.getString("logger", "account", "ERROR");
  auto logger_loginserver = config.getString("logger", "loginserver", "ERROR");
  auto logger_network     = config.getString("logger", "network", "ERROR");
  auto logger_utils       = config.getString("logger", "utils", "ERROR");

  // Set logger settings
  utils::Logger::setLevel("account",     logger_account);
  utils::Logger::setLevel("loginserver", logger_loginserver);
  utils::Logger::setLevel("network",     logger_network);
  utils::Logger::setLevel("utils",       logger_utils);

  // Print configuration values
  printf("--------------------------------------------------------------------------------\n");
  printf("LoginServer configuration\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("Server port:               %d\n", server_port);
  printf("Websocket server port:     %d\n", ws_server_port);
  printf("\n");
  printf("Accounts filename:         %s\n", accounts_filename.c_str());
  printf("Message of the day:        %s\n", motd.motd.c_str());
  printf("\n");
  printf("Account logging:           %s\n", logger_account.c_str());
  printf("Loginserver logging:       %s\n", logger_loginserver.c_str());
  printf("Network logging:           %s\n", logger_network.c_str());
  printf("Utils logging:             %s\n", logger_utils.c_str());
  printf("--------------------------------------------------------------------------------\n");

  // Create io_context
  asio::io_context io_context;

  // Create and load AccountReader
  account_reader = std::make_unique<account::AccountReader>();
  if (!account_reader->load(accounts_filename))
  {
    LOG_ERROR("Could not load accounts file: %s", accounts_filename.c_str());
    account_reader.reset();
    return 1;
  }

  // Create Server
  server = network::ServerFactory::createServer(&io_context, server_port, &onClientConnected);

  // Create websocket server
  websocket_server = network::ServerFactory::createWebsocketServer(&io_context, ws_server_port, &onClientConnected);

  LOG_INFO("LoginServer started!");

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

  LOG_INFO("Stopping LoginServer!");

  // Deallocate things
  websocket_server.reset();
  server.reset();
  account_reader.reset();

  return 0;
}
