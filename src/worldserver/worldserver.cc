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

#include <deque>
#include <functional>
#include <memory>
#include <boost/asio.hpp>  //NOLINT

#include "configparser.h"
#include "logger.h"
#include "account.h"
#include "server.h"
#include "incomingpacket.h"
#include "outgoingpacket.h"
#include "direction.h"
#include "gameengine.h"

// Globals
AccountReader accountReader;
std::unique_ptr<Server> server;
std::unique_ptr<GameEngine> gameEngine;
std::unordered_map<ConnectionId, CreatureId> players;

// Handlers for Server
void onClientConnected(ConnectionId connectionId);
void onClientDisconnected(ConnectionId connectionId);
void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet);

// Parse functions
void parseLogin(ConnectionId connectionId, IncomingPacket* packet);
void parseMoveClick(CreatureId playerId, IncomingPacket* packet);
void parseMoveItem(CreatureId playerId, IncomingPacket* packet);
void parseUseItem(CreatureId playerId, IncomingPacket* packet);
void parseLookAt(CreatureId playerId, IncomingPacket* packet);
void parseSay(CreatureId playerId, IncomingPacket* packet);
void parseCancelMove(CreatureId playerId, IncomingPacket* packet);

// Callback for GameEngine (PlayerCtrl)
void sendPacket(ConnectionId connectionId, OutgoingPacket&& packet);

// Helper functions
Position getPosition(IncomingPacket* packet);

void onClientConnected(ConnectionId connectionId)
{
  LOG_DEBUG("Client connected, id: %d", connectionId);
}

void onClientDisconnected(ConnectionId connectionId)
{
  LOG_DEBUG("Client disconnected, id: %d", connectionId);

  // Check if the connection is logged in to the game engine
  auto playerIt = players.find(connectionId);
  if (playerIt != players.end())
  {
    gameEngine->playerDespawn(playerIt->second);
    players.erase(connectionId);
  }
}

void onPacketReceived(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("Parsing packet from connection id: %d, packet size: %d", connectionId, packet->getLength());

  // Check if the connection is logged in to the game engine
  auto playerIt = players.find(connectionId);
  if (playerIt == players.end())
  {
    // Not logged in, we only accept the login packet (0x0A) here
    uint8_t packetId = packet->getU8();
    if (packetId != 0x0A)
    {
      LOG_ERROR("Unexpected packet from connection id: %d. Expected login packet, not: 0x%X", connectionId, packetId);
      server->closeConnection(connectionId);
      return;
    }

    // Login packet
    parseLogin(connectionId, packet);
    return;
  }

  // The connection is logged in, handle the packet
  CreatureId playerId = playerIt->second;
  while (!packet->isEmpty())
  {
    uint8_t packetId = packet->getU8();
    switch (packetId)
    {
      case 0x14:  // Logout
      {
        gameEngine->playerDespawn(playerId);
        players.erase(connectionId);
        server->closeConnection(connectionId);
        return;
      }

      case 0x64:  // Player move with mouse
      {
        parseMoveClick(playerId, packet);
        break;
      }

      case 0x65:  // Player move, North = 0
      case 0x66:  // East  = 1
      case 0x67:  // South = 2
      case 0x68:  // West  = 3
      {
        gameEngine->playerMove(playerId, static_cast<Direction>(packetId - 0x65));
        break;
      }

      case 0x6F:  // Player turn, North = 0
      case 0x70:  // East  = 1
      case 0x71:  // South = 2
      case 0x72:  // West  = 3
      {
        gameEngine->playerTurn(playerId, static_cast<Direction>(packetId - 0x6F));
        break;
      }

      case 0x78:  // Move item
      {
        parseMoveItem(playerId, packet);
        break;
      }

      case 0x82:  // Use item
      {
        parseUseItem(playerId, packet);
        break;
      }

      case 0x8C:  // Look at
      {
        parseLookAt(playerId, packet);
        break;
      }

      case 0x96:
      {
        parseSay(playerId, packet);
        break;
      }

      case 0xBE:
      {
        parseCancelMove(playerId, packet);
        break;
      }

      default:
      {
        LOG_ERROR("Unknown packet from connection id: %d, packet id: 0x%X", connectionId, packetId);
        return;  // Don't read any more, even though there might be more packets that we can parse
      }
    }
  }
}

void parseLogin(ConnectionId connectionId, IncomingPacket* packet)
{
  LOG_DEBUG("Parsing packet from connection id: %d", connectionId);

  packet->getU8();  // Unknown (0x02)
  uint8_t client_os = packet->getU8();
  uint16_t client_version = packet->getU16();
  packet->getU8();  // Unknown
  std::string character_name = packet->getString();
  std::string password = packet->getString();

  LOG_DEBUG("Client OS: %d Client version: %d Character: %s Password: %s",
              client_os,
              client_version,
              character_name.c_str(),
              password.c_str());

  // Check if character exists
  if (!accountReader.characterExists(character_name.c_str()))
  {
    OutgoingPacket response;
    response.addU8(0x14);
    response.addString("Invalid character.");
    server->sendPacket(connectionId, std::move(response));
    server->closeConnection(connectionId);
    return;
  }
  // Check if password is correct
  else if (!accountReader.verifyPassword(character_name, password))
  {
    OutgoingPacket response;
    response.addU8(0x14);
    response.addString("Invalid password.");
    server->sendPacket(connectionId, std::move(response));
    server->closeConnection(connectionId);
    return;
  }

  // Login OK
  auto sendPacketFunc = std::bind(&sendPacket, connectionId, std::placeholders::_1);
  CreatureId playerId = gameEngine->playerSpawn(character_name, sendPacketFunc);

  // Store the playerId
  players.insert(std::make_pair(connectionId, playerId));
}

void parseMoveClick(CreatureId playerId, IncomingPacket* packet)
{
  std::deque<Direction> moves;
  uint8_t pathLength = packet->getU8();

  if (pathLength == 0)
  {
    LOG_ERROR("%s: Path length is zero!", __func__);
    return;
  }

  for (auto i = 0; i < pathLength; i++)
  {
    moves.push_back(static_cast<Direction>(packet->getU8()));
  }

  gameEngine->playerMovePath(playerId, moves);
}

void parseMoveItem(CreatureId playerId, IncomingPacket* packet)
{
  // There are four options here:
  // Moving from inventory to inventory
  // Moving from inventory to Tile
  // Moving from Tile to inventory
  // Moving from Tile to Tile
  if (packet->peekU16() == 0xFFFF)
  {
    // Moving from inventory ...
    packet->getU16();

    auto fromInventoryId = packet->getU8();
    auto unknown = packet->getU16();
    auto itemId = packet->getU16();
    auto unknown2 = packet->getU8();

    if (packet->peekU16() == 0xFFFF)
    {
      // ... to inventory
      packet->getU16();
      auto toInventoryId = packet->getU8();
      auto unknown3 = packet->getU16();
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("parseMoveItem(): Moving %d (countOrSubType %d) from inventoryId %d to iventoryId %d (unknown %d, %d, %d)",
                  itemId, countOrSubType, fromInventoryId, toInventoryId, unknown, unknown2, unknown3);


      gameEngine->playerMoveItemFromInvToInv(playerId, fromInventoryId, itemId, countOrSubType, toInventoryId);
    }
    else
    {
      // ... to Tile
      auto toPosition = getPosition(packet);
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("parseMoveItem(): Moving %d (countOrSubType %d) from inventoryId %d to %s (unknown %d, %d)",
                  itemId, countOrSubType, fromInventoryId, toPosition.toString().c_str(), unknown, unknown2);


      gameEngine->playerMoveItemFromInvToPos(playerId, fromInventoryId, itemId, countOrSubType, toPosition);
    }
  }
  else
  {
    // Moving from Tile ...
    auto fromPosition = getPosition(packet);
    auto itemId = packet->getU16();
    auto fromStackPos = packet->getU8();

    if (packet->peekU16() == 0xFFFF)
    {
      // ... to inventory
      packet->getU16();

      auto toInventoryId = packet->getU8();
      auto unknown = packet->getU16();
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("parseMoveItem(): Moving %d (countOrSubType %d) from %s (stackpos: %d) to inventoryId %d (unknown: %d)",
                  itemId, countOrSubType, fromPosition.toString().c_str(), fromStackPos, toInventoryId, unknown);

      gameEngine->playerMoveItemFromPosToInv(playerId, fromPosition, fromStackPos, itemId, countOrSubType, toInventoryId);
    }
    else
    {
      // ... to Tile
      auto toPosition = getPosition(packet);
      auto countOrSubType = packet->getU8();

      LOG_DEBUG("parseMoveItem(): Moving %d (countOrSubType %d) from %s (stackpos: %d) to %s (unknown: %d)",
                  itemId, countOrSubType, fromPosition.toString().c_str(), fromStackPos, toPosition.toString().c_str());

      gameEngine->playerMoveItemFromPosToPos(playerId, fromPosition, fromStackPos, itemId, countOrSubType, toPosition);
    }
  }
}

void parseUseItem(CreatureId playerId, IncomingPacket* packet)
{
  // There are two options here:
  if (packet->peekU16() == 0xFFFF)
  {
    // Use Item in inventory
    packet->getU16();
    auto inventoryIndex = packet->getU8();
    auto unknown = packet->getU16();
    auto itemId = packet->getU16();
    auto unknown2 = packet->getU16();

    LOG_DEBUG("parseUseItem(): Using Item %d at inventory index: %d (unknown: %d, unknown2: %d)",
                itemId, inventoryIndex, unknown, unknown2);

    gameEngine->playerUseInvItem(playerId, itemId, inventoryIndex);
  }
  else
  {
    // Use Item on Tile
    auto position = getPosition(packet);
    auto itemId = packet->getU16();
    auto stackPosition = packet->getU8();
    auto unknown = packet->getU8();

    LOG_DEBUG("parseUseItem(): Using Item %d at Tile: %s stackPos: %d (unknown: %d)",
                itemId, position.toString().c_str(), stackPosition, unknown);

    gameEngine->playerUsePosItem(playerId, itemId, position, stackPosition);
  }
}

void parseLookAt(CreatureId playerId, IncomingPacket* packet)
{
  // TODO(gurka): Look at inventory / container?
  Position position = getPosition(packet);
  uint16_t itemId = packet->getU16();

  gameEngine->playerLookAt(playerId, position, itemId);
}

void parseSay(CreatureId playerId, IncomingPacket* packet)
{
  uint8_t type = packet->getU8();

  std::string receiver = "";
  uint16_t channelId = 0;

  switch (type)
  {
    case 0x06:  // PRIVATE
    case 0x0B:  // PRIVATE RED
      receiver = packet->getString();
      break;
    case 0x07:  // CHANNEL_Y
    case 0x0A:  // CHANNEL_R1
      channelId = packet->getU16();
      break;
    default:
      break;
  }

  std::string message = packet->getString();

  gameEngine->playerSay(playerId, type, message, receiver, channelId);
}

void parseCancelMove(CreatureId playerId, IncomingPacket* packet)
{
  gameEngine->playerCancelMove(playerId);
}

void sendPacket(int connectionId, OutgoingPacket&& packet)
{
  server->sendPacket(connectionId, std::move(packet));
}

Position getPosition(IncomingPacket* packet)
{
  auto x = packet->getU16();
  auto y = packet->getU16();
  auto z = packet->getU8();
  return Position(x, y, z);
}

int main(int argc, char* argv[])
{
  // Read configuration
  auto config = ConfigParser::parseFile("data/worldserver.cfg");
  if (!config.parsedOk())
  {
    LOG_INFO("Could not parse config file: %s", config.getErrorMessage().c_str());
    LOG_INFO("Will continue with default values");
  }

  auto serverPort = config.getInteger("server", "port", 7172);

  auto loginMessage = config.getString("world", "login_message", "Welcome to LoginServer!");
  auto accountsFilename = config.getString("world", "accounts_file", "data/accounts.xml");
  auto dataFilename = config.getString("world", "data_file", "data/data.dat");
  auto itemsFilename = config.getString("world", "item_file", "data/items.xml");
  auto worldFilename = config.getString("world", "world_file", "data/world.xml");

  // Print configuration values
  LOG_INFO("                            WorldServer configuration                           ");
  LOG_INFO("================================================================================");
  LOG_INFO("Server port:               %d", serverPort);
  LOG_INFO("");
  LOG_INFO("Login message:             %s", loginMessage.c_str());
  LOG_INFO("Accounts filename:         %s", accountsFilename.c_str());
  LOG_INFO("Data filename:             %s", dataFilename.c_str());
  LOG_INFO("Items filename:            %s", itemsFilename.c_str());
  LOG_INFO("World filename:            %s", worldFilename.c_str());
  LOG_INFO("================================================================================");

  // Setup io_service, AccountManager, GameEngine and Server
  boost::asio::io_service io_service;

  Server::Callbacks callbacks =
  {
    &onClientConnected,
    &onClientDisconnected,
    &onPacketReceived,
  };
  server = std::unique_ptr<Server>(new Server(&io_service, serverPort, callbacks));
  gameEngine = std::unique_ptr<GameEngine>(new GameEngine(&io_service,
                                                          loginMessage,
                                                          dataFilename,
                                                          itemsFilename,
                                                          worldFilename));
  if (!accountReader.loadFile(accountsFilename))
  {
    LOG_ERROR("Could not load accounts file: %s", accountsFilename.c_str());
    return 1;
  }

  boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
  signals.async_wait(std::bind(&boost::asio::io_service::stop, &io_service));

  // Start Server, GameEngine and io_service
  if (!server->start())
  {
    LOG_ERROR("Could not start Server");
    return -1;
  }

  if (!gameEngine->start())
  {
    LOG_ERROR("Could not start GameEngine");
    return -2;
  }

  // run() will continue to run until ^C from user is catched
  io_service.run();

  LOG_INFO("Stopping GameEngine");
  gameEngine->stop();

  LOG_INFO("Stopping Server");
  server->stop();

  return 0;
}
