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

#ifndef WORLD_EXPORT_WORLD_H_
#define WORLD_EXPORT_WORLD_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "world_interface.h"
#include "creature.h"
#include "creature_ctrl.h"
#include "item.h"
#include "tile.h"
#include "position.h"

namespace world
{

class World : public WorldInterface
{
 public:
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

  World(int world_size_x,
        int world_size_y,
        std::vector<Tile>&& tiles);
  ~World() override;

  // Delete copy constructors
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  // Creature management
  ReturnCode addCreature(Creature* creature, CreatureCtrl* creature_ctrl, const Position& position);
  void removeCreature(CreatureId creature_id);
  bool creatureExists(CreatureId creature_id) const;
  ReturnCode creatureMove(CreatureId creature_id, Direction direction);
  ReturnCode creatureMove(CreatureId creature_id, const Position& to_position);
  void creatureTurn(CreatureId creature_id, Direction direction);
  void creatureSay(CreatureId creature_id, const std::string& message);

  // Item management
  bool canAddItem(const Item& item, const Position& position) const;
  ReturnCode addItem(const Item& item, const Position& position);
  ReturnCode removeItem(ItemTypeId item_type_id, int count, const Position& position, int stackpos);
  ReturnCode moveItem(CreatureId creature_id,
                      const Position& from_position,
                      int from_stackpos,
                      ItemTypeId item_type_id,
                      int count,
                      const Position& to_position);

  // Creature checks
  bool creatureCanThrowTo(CreatureId creature_id, const Position& position) const;
  bool creatureCanReach(CreatureId creature_id, const Position& position) const;

  // WorldInterface
  const Tile* getTile(const Position& position) const override;
  const Creature& getCreature(CreatureId creature_id) const override;
  const Position& getCreaturePosition(CreatureId creature_id) const override;

 private:
  // Functions to use instead of accessing the containers directly
  Tile* getTile(const Position& position);
  Creature& getCreature(CreatureId creature_id);
  CreatureCtrl& getCreatureCtrl(CreatureId creature_id);

  // Helper functions
  std::vector<CreatureId> getCreatureIdsThatCanSeePosition(const Position& position) const;
  int getCreatureStackpos(const Position& position, CreatureId creature_id) const;

  // World size
  int m_world_size_x;
  int m_world_size_y;

  // Column-major order, due to how map blocks are sent to client
  // No z axis yet
  // index = (((x - POSITION_OFFSET) * m_world_size_y) + (y - POSITION_OFFSET))
  std::vector<Tile> m_tiles;

  struct CreatureData
  {
    CreatureData(Creature* creature,
                 CreatureCtrl* creature_ctrl,
                 const Position& position)
      : creature(creature),
        creature_ctrl(creature_ctrl),
        position(position)
    {
    }

    Creature* creature;
    CreatureCtrl* creature_ctrl;
    Position position;
  };
  std::unordered_map<CreatureId, CreatureData> m_creature_data;
};

}  // namespace world

#endif  // WORLD_EXPORT_WORLD_H_
