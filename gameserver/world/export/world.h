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

#ifndef WORLD_WORLD_H_
#define WORLD_WORLD_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "world_interface.h"
#include "creature.h"
#include "creature_ctrl.h"
#include "tile.h"
#include "position.h"

class World : public WorldInterface
{
 public:
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

  World(int worldSizeX,
        int worldSizeY,
        std::unordered_map<Position, Tile, Position::Hash> tiles);

  // Delete copy constructors
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  // Creature management
  ReturnCode addCreature(Creature* creature, CreatureCtrl* creatureCtrl, const Position& position);
  void removeCreature(CreatureId creatureId);
  bool creatureExists(CreatureId creatureId) const;
  ReturnCode creatureMove(CreatureId creatureId, Direction direction);
  ReturnCode creatureMove(CreatureId creatureId, const Position& newPosition);
  void creatureTurn(CreatureId creatureId, Direction direction);
  void creatureSay(CreatureId creatureId, const std::string& message);

  // Item management
  ReturnCode addItem(ItemId itemId, const Position& position);
  ReturnCode addItem(const Item& item, const Position& position);
  ReturnCode removeItem(int itemId, int count, const Position& position, int stackPos);
  ReturnCode moveItem(CreatureId creatureId, const Position& fromPosition, int fromStackPos,
                      int itemId, int count, const Position& toPosition);

  // Creature checks
  bool creatureCanThrowTo(CreatureId creatureId, const Position& position) const;
  bool creatureCanReach(CreatureId creatureId, const Position& position) const;

  // WorldInterface
  const std::vector<const Tile*> getMapBlock(const Position& position, int width, int height) const override;
  const Tile& getTile(const Position& position) const override;
  const Creature& getCreature(CreatureId creatureId) const override;
  const Position& getCreaturePosition(CreatureId creatureId) const override;

 private:
  // Validation functions
  bool positionIsValid(const Position& position) const;

  // Helper functions
  std::vector<CreatureId> getVisibleCreatureIds(const Position& position) const;

  // Functions to use instead of accessing the unordered_maps directly
  Tile& internalGetTile(const Position& position);
  Creature& internalGetCreature(CreatureId creatureId);
  CreatureCtrl& getCreatureCtrl(CreatureId creatureId);

  // World size
  int worldSizeX_;
  int worldSizeY_;

  // Offset for world size, since the client doesn't like too low positions
  const int worldSizeStart_ = 192;

  // TODO(simon): Use std::vector or std::array since pointer access is slow,
  //              and contiguous memory is cache friendly.
  //              Changing tiles_ to vector/array is not easy:
  //                Make sure access is always done in same order, e.g.
  //                for each z:
  //                  for each y:
  //                    for each x:
  //                Figure out limits (should be easy, given worldSizeX/Y_ & worldSizeStart_)
  //              Profile!
  //              Add profiler test case that does lots of actions in the world?
  //                (Moves around creatures / items, non-randomly)
  std::unordered_map<Position, Tile, Position::Hash> tiles_;
  std::unordered_map<int, Creature*> creatures_;
  std::unordered_map<int, CreatureCtrl*> creatureCtrls_;
  std::unordered_map<int, Position> creaturePositions_;
};

#endif  // WORLD_WORLD_H_
