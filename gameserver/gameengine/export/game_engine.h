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

#ifndef GAMEENGINE_EXPORT_GAME_ENGINE_H_
#define GAMEENGINE_EXPORT_GAME_ENGINE_H_

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "world.h"
#include "player.h"
#include "position.h"
#include "game_position.h"

class OutgoingPacket;

namespace gameengine
{

class GameEngineQueue;
class PlayerCtrl;
class ItemManager;
class ContainerManager;

class GameEngine
{
 public:
  GameEngine();
  ~GameEngine();

  // Delete copy constructors
  GameEngine(const GameEngine&) = delete;
  GameEngine& operator=(const GameEngine&) = delete;

  bool init(GameEngineQueue* game_engine_queue,
            const std::string& login_message,
            const std::string& data_filename,
            const std::string& items_filename,
            const std::string& world_filename);
  const world::World* getWorld() const { return m_world.get(); }

  bool spawn(const std::string& name, PlayerCtrl* player_ctrl);
  void despawn(world::CreatureId creature_id);

  void move(world::CreatureId creature_id, world::Direction direction);
  void movePath(world::CreatureId creature_id, std::deque<world::Direction>&& path);
  void cancelMove(world::CreatureId creature_id);
  void turn(world::CreatureId creature_id, world::Direction direction);

  void say(world::CreatureId creature_id,
           int type,
           const std::string& message,
           const std::string& receiver,
           int channel_id);

  void moveItem(world::CreatureId creature_id, const ItemPosition& from_position, const GamePosition& to_position, int count);
  void useItem(world::CreatureId creature_id, const ItemPosition& position, int new_container_id);
  void lookAt(world::CreatureId creature_id, const ItemPosition& position);

  void closeContainer(world::CreatureId creature_id, world::ItemUniqueId item_unique_id);
  void openParentContainer(world::CreatureId creature_id, world::ItemUniqueId item_unique_id, int new_container_id);

 private:
  const world::Item* getItem(world::CreatureId creature_id, const ItemPosition& position);
  bool canAddItem(world::CreatureId creature_id, const GamePosition& position, const world::Item& item, int count);
  void removeItem(world::CreatureId creature_id, const ItemPosition& position, int count);
  void addItem(world::CreatureId creature_id, const GamePosition& position, const world::Item& item, int count);

  // This structure holds all player data that shouldn't go into Player
  struct PlayerData
  {
    PlayerData(Player&& player, PlayerCtrl* player_ctrl)
      : player(std::move(player)),
        player_ctrl(player_ctrl)
    {
    }

    Player player;
    PlayerCtrl* player_ctrl;
    std::deque<world::Direction> queued_moves;
  };

  // Use these instead of the unordered_map directly
  PlayerData& getPlayerData(world::CreatureId creature_id) { return m_player_data.at(creature_id); }
  const PlayerData& getPlayerData(world::CreatureId creature_id) const { return m_player_data.at(creature_id); }

  std::unordered_map<world::CreatureId, PlayerData> m_player_data;
  std::unique_ptr<ItemManager> m_item_manager;
  std::unique_ptr<world::World> m_world;
  GameEngineQueue* m_game_engine_queue{nullptr};
  std::string m_login_message;
  std::unique_ptr<ContainerManager> m_container_manager;
};

}  // namespace gameengine

#endif  // GAMEENGINE_EXPORT_GAME_ENGINE_H_
