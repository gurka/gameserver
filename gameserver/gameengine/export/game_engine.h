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
#include "item_manager.h"
#include "player.h"
#include "position.h"
#include "container_manager.h"
#include "game_position.h"

class GameEngineQueue;
class OutgoingPacket;
class PlayerCtrl;

class GameEngine
{
 public:
  GameEngine()
    : gameEngineQueue_(nullptr)
  {
  }

  // Delete copy constructors
  GameEngine(const GameEngine&) = delete;
  GameEngine& operator=(const GameEngine&) = delete;

  bool init(GameEngineQueue* gameEngineQueue,
            const std::string& loginMessage,
            const std::string& dataFilename,
            const std::string& itemsFilename,
            const std::string& worldFilename);

  bool spawn(const std::string& name, PlayerCtrl* player_ctrl);
  void despawn(CreatureId creatureId);

  void move(CreatureId creatureId, Direction direction);
  void movePath(CreatureId creatureId, std::deque<Direction>&& path);
  void cancelMove(CreatureId creatureId);
  void turn(CreatureId creatureId, Direction direction);

  void say(CreatureId creatureId,
           int type,
           const std::string& message,
           const std::string& receiver,
           int channelId);

  void moveItem(CreatureId creatureId, const ItemPosition& fromPosition, const GamePosition& toPosition, int count);
  void useItem(CreatureId creatureId, const ItemPosition& position, int newContainerId);
  void lookAt(CreatureId creatureId, const ItemPosition& position);

  void closeContainer(CreatureId creatureId, ItemUniqueId itemUniqueId);
  void openParentContainer(CreatureId creatureId, ItemUniqueId itemUniqueId, int newContainerId);

 private:
  Item* getItem(CreatureId creatureId, const ItemPosition& position);
  bool canAddItem(CreatureId creatureId, const GamePosition& position, const Item& item, int count);
  void removeItem(CreatureId creatureId, const ItemPosition& position, int count);
  void addItem(CreatureId creatureId, const GamePosition& position, Item* item, int count);

  // This structure holds all player data that shouldn't go into Player
  struct PlayerData
  {
    PlayerData(Player&& player, PlayerCtrl* player_ctrl)
      : player(std::move(player)),
        player_ctrl(player_ctrl),
        queued_moves()
    {
    }

    Player player;
    PlayerCtrl* player_ctrl;
    std::deque<Direction> queued_moves;
  };

  // Use these instead of the unordered_map directly
  PlayerData& getPlayerData(CreatureId creatureId) { return playerData_.at(creatureId); }
  const PlayerData& getPlayerData(CreatureId creatureId) const { return playerData_.at(creatureId); }

  std::unordered_map<CreatureId, PlayerData> playerData_;

  // TODO(simon): refactor away unique_ptr
  std::unique_ptr<World> world_;

  GameEngineQueue* gameEngineQueue_;
  std::string loginMessage_;
  ItemManager itemManager_;
  ContainerManager containerManager_;
};

#endif  // GAMEENGINE_EXPORT_GAME_ENGINE_H_
