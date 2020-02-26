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

#ifndef WORLD_EXPORT_WORLD_H_
#define WORLD_EXPORT_WORLD_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "creature.h"
#include "creature_ctrl.h"
#include "item.h"
#include "tile.h"
#include "position.h"

namespace world
{

static constexpr int POSITION_OFFSET = 192;

enum class ReturnCode
{
  OK,
  INVALID_CREATURE,
  INVALID_POSITION,
  ITEM_NOT_FOUND,
  CANNOT_MOVE_THAT_OBJECT,
  CANNOT_REACH_THAT_OBJECT,
  THERE_IS_NO_ROOM,
  MAY_NOT_MOVE_YET,
  OTHER_ERROR,
};

class World
{
 public:
  World(int world_size_x,
        int world_size_y,
        std::vector<Tile>&& tiles);
  virtual ~World();

  // Delete copy constructors
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  // Creature management
  ReturnCode addCreature(common::Creature* creature, CreatureCtrl* creature_ctrl, const common::Position& position);
  void removeCreature(common::CreatureId creature_id);
  bool creatureExists(common::CreatureId creature_id) const;
  ReturnCode creatureMove(common::CreatureId creature_id, common::Direction direction);
  ReturnCode creatureMove(common::CreatureId creature_id, const common::Position& to_position);
  void creatureTurn(common::CreatureId creature_id, common::Direction direction);
  void creatureSay(common::CreatureId creature_id, const std::string& message);
  const common::Position* getCreaturePosition(common::CreatureId creature_id) const;
  bool creatureCanThrowTo(common::CreatureId creature_id, const common::Position& position) const;
  bool creatureCanReach(common::CreatureId creature_id, const common::Position& position) const;

  // Item management
  bool canAddItem(const common::Item& item, const common::Position& position) const;
  ReturnCode addItem(const common::Item& item, const common::Position& position);
  ReturnCode removeItem(common::ItemTypeId item_type_id, int count, const common::Position& position, int stackpos);
  ReturnCode moveItem(common::CreatureId creature_id,
                      const common::Position& from_position,
                      int from_stackpos,
                      common::ItemTypeId item_type_id,
                      int count,
                      const common::Position& to_position);

  // Tile management
  const Tile* getTile(const common::Position& position) const;

 private:
  // Functions to use instead of accessing the containers directly
  Tile* getTile(const common::Position& position);
  common::Creature* getCreature(common::CreatureId creature_id);
  CreatureCtrl& getCreatureCtrl(common::CreatureId creature_id);

  // Helper functions
  std::vector<common::CreatureId> getCreatureIdsThatCanSeePosition(const common::Position& position) const;
  int getCreatureStackpos(const common::Position& position, common::CreatureId creature_id) const;

  // World size
  int m_world_size_x;
  int m_world_size_y;

  // Column-major order, due to how map blocks are sent to client
  // No z axis yet
  // index = (((x - POSITION_OFFSET) * m_world_size_y) + (y - POSITION_OFFSET))
  std::vector<Tile> m_tiles;

  struct CreatureData
  {
    CreatureData(common::Creature* creature,
                 CreatureCtrl* creature_ctrl,
                 const common::Position& position)
      : creature(creature),
        creature_ctrl(creature_ctrl),
        position(position)
    {
    }

    common::Creature* creature;
    CreatureCtrl* creature_ctrl;
    common::Position position;
  };
  std::unordered_map<common::CreatureId, CreatureData> m_creature_data;
};

}  // namespace world

#endif  // WORLD_EXPORT_WORLD_H_
