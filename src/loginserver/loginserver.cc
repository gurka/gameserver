/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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
#include <boost/asio.hpp>  //NOLINT

#include "configparser.h"
#include "logger.h"
#include "account/account.h"
#include "network/server.h"
#include "network/incomingpacket.h"
#include "network/outgoingpacket.h"

void onClientConnected(ConnectionId connectionId);
void onClientDisconnected(ConnectionId connectionId);
void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet);
void parseLogin(ConnectionId connectionId, IncomingPacket* packet);

AccountReader accountReader;
std::unique_ptr<Server> server;
std::string motd;

void onClientConnected(ConnectionId connectionId)
{
  LOG_DEBUG("Client connected, id: %d", connectionId);
}

void onClientDisconnected(ConnectionId connectionId)
{
  LOG_DEBUG("Client disconnected, id: %d", connectionId);
}

void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("Parsing packet from connection id: %d", connectionId);

  while (!packet->isEmpty())
  {
    uint8_t packetId = packet->getU8();
    switch (packetId)
    {
      case 0x01:
      {
        parseLogin(connectionId, packet);
        break;
      }

      default:
      {
        LOG_DEBUG("Unknown packet from connection id: %d, packet id: %d", connectionId, packetId);
        server->closeConnection(connectionId);
        break;
      }
    }
  }
}

void parseLogin(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("Parsing login packet from connection id: %d", connectionId);

  uint16_t clientOs = packet->getU16();       // Client OS
  uint16_t clientVersion = packet->getU16();  // Client version
  packet->getBytes(12);                       // Client OS info
  uint32_t accountNumber = packet->getU32();
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
  response.addString("0\n" + motd);

  // Check if account exists
  if (!accountReader.accountExists(accountNumber))
  {
    LOG_DEBUG("%s: Account (%d) not found", __func__, accountNumber);
    response.addU8(0x0A);
    response.addString("Invalid account number");
  }
  // Check if password is correct
  else if (!accountReader.verifyPassword(accountNumber, password))
  {
    LOG_DEBUG("%s: Invalid password (%s) for account (%d)", __func__, password.c_str(), accountNumber);
    response.addU8(0x0A);
    response.addString("Invalid password");
  }
  else
  {
    const auto* account = accountReader.getAccount(accountNumber);
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
  server->sendPacket(connectionId, response);

  LOG_DEBUG("Closing connection id: %d", connectionId);
  server->closeConnection(connectionId);
}

int main(int argc, char* argv[])
{
  // Read configuration
  auto config = ConfigParser::parseFile("data/loginserver.cfg");
  if (!config.parsedOk())
  {
    LOG_INFO("Could not parse config file: %s", config.getErrorMessage().c_str());
    LOG_INFO("Will continue with default values");
  }

  auto serverPort = config.getInteger("server", "port", 7171);

  motd = config.getString("login", "motd", "Welcome to LoginServer!");
  auto accountsFilename = config.getString("login", "accounts_file", "data/accounts.xml");

  // Print configuration values
  LOG_INFO("                            LoginServer configuration                           ");
  LOG_INFO("================================================================================");
  LOG_INFO("Server port:               %d", serverPort);
  LOG_INFO("");
  LOG_INFO("Message of the day:        %s", motd.c_str());
  LOG_INFO("Accounts filename:         %s", accountsFilename.c_str());
  LOG_INFO("================================================================================");

  // Setup io_service, AccountManager and Server
  boost::asio::io_service io_service;

  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
  signals.async_wait(std::bind(&boost::asio::io_service::stop, &io_service));

  if (!accountReader.loadFile(accountsFilename))
  {
    LOG_ERROR("Could not load accounts file: %s", accountsFilename.c_str());
    return 1;
  }

  Server::Callbacks callbacks =
  {
    &onClientConnected,
    &onClientDisconnected,
    &onPacketReceived,
  };
  server = std::unique_ptr<Server>(new Server(&io_service, serverPort, callbacks));

  // Start Server and io_service
  if (!server->start())
  {
    LOG_ERROR("Could not start Server");
    return 1;
  }

  // run() will continue to run until ^C from user is catched
  io_service.run();

  LOG_INFO("Stopping server");
  server->stop();

  return 0;
}
