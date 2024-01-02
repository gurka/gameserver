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

#ifndef WSCLIENT_SRC_PROTOCOL_H_
#define WSCLIENT_SRC_PROTOCOL_H_

namespace network
{
class IncomingPacket;
}

namespace chat
{
class Chat;
}

namespace game
{
class Game;
}

namespace sidebar
{
class Sidebar;
}

namespace protocol::client
{
struct Login;
struct LoginFailed;
struct FullMap;
struct PartialMap;
struct TileUpdate;
struct FloorChange;
struct MagicEffect;
struct PlayerStats;
struct WorldLight;
struct PlayerSkills;
struct Equipment;
struct TextMessage;
struct ThingAdded;
struct ThingChanged;
struct ThingMoved;
struct ThingRemoved;
struct CreatureSkull;
}

/**
 * Reads IncomingPackets and updates models (game, chat, player info, etc.)
 */
class Protocol
{
 public:
  Protocol(game::Game* game, chat::Chat* chat, sidebar::Sidebar* sidebar);

  void handlePacket(network::IncomingPacket* packet);

 private:
  void handleLoginPacket(const protocol::client::Login& login);
  void handleLoginFailedPacket(const protocol::client::LoginFailed& failed);
  void handleFullMapPacket(const protocol::client::FullMap& map_data);
  void handlePartialMapPacket(const protocol::client::PartialMap& map_data);
  void handleTileUpdatePacket(const protocol::client::TileUpdate& tile_update);
  void handleFloorChange(bool up, const protocol::client::FloorChange& floor_change);
  void handleMagicEffect(const protocol::client::MagicEffect& effect);
  void handlePlayerStats(const protocol::client::PlayerStats& stats);
  void handleWorldLight(const protocol::client::WorldLight& light);
  void handlePlayerSkills(const protocol::client::PlayerSkills& skills);
  void handleEquipmentUpdate(const protocol::client::Equipment& equipment);
  void handleTextMessage(const protocol::client::TextMessage& message);
  void handleThingAdded(const protocol::client::ThingAdded& thing_added);
  void handleThingChanged(const protocol::client::ThingChanged& thing_changed);
  void handleThingMoved(const protocol::client::ThingMoved& thing_moved);
  void handleThingRemoved(const protocol::client::ThingRemoved& thing_removed);
  void handleCreatureSkull(const protocol::client::CreatureSkull& creature_skull);

  int m_num_handled_packets;
  game::Game* m_game;
  chat::Chat* m_chat;
  sidebar::Sidebar* m_sidebar;
};

#endif  // WSCLIENT_SRC_PROTOCOL_H_
