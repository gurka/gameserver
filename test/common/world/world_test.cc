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

#include <memory>
#include <unordered_map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mocks/creaturectrl_mock.h"
#include "world.h"
#include "creature.h"
#include "creaturectrl.h"
#include "position.h"
#include "item.h"

using ::testing::AtLeast;
using ::testing::_;

class WorldTest : public ::testing::Test
{
 protected:
  WorldTest()
  {
    std::unordered_map<Position, Tile, Position::Hash> tiles;
    for (auto x = 0; x < 16; x++)
    {
      for (auto y = 0; y < 16; y++)
      {
        // TODO(gurka): This damn 192 constant again...
        tiles.insert(std::make_pair(Position(192 + x, 192 + y, 7),
                                    Tile(Item())));
      }
    }

    world.reset(new World(nullptr, 16, 16, tiles));
  }

  std::unique_ptr<World> world;
};

TEST_F(WorldTest, AddCreature)
{
  // Add first Creature
  Creature creatureOne("TestCreatureOne");
  MockCreatureCtrl creatureCtrlOne;
  Position creaturePositionOne(192, 192, 7);

  // Adding creatureOne should not call creatureOne's CreatureCtrl::onCreatureSpawn
  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _)).Times(0);
  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);

  EXPECT_TRUE(world->creatureExists(creatureOne.getCreatureId()));
  EXPECT_EQ(creatureOne, world->getCreature(creatureOne.getCreatureId()));
  EXPECT_EQ(creaturePositionOne, world->getCreaturePosition(creatureOne.getCreatureId()));

  // Add second Creature
  Creature creatureTwo("TestCreatureTwo");
  MockCreatureCtrl creatureCtrlTwo;
  Position creaturePositionTwo(193, 192, 7);

  // Addding creatureTwo should call creatureOne's CreatureCtrl::onCreatureSpawn
  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(creatureTwo, creaturePositionTwo)).Times(1);
  world->addCreature(&creatureTwo, &creatureCtrlTwo, creaturePositionTwo);

  EXPECT_TRUE(world->creatureExists(creatureTwo.getCreatureId()));
  EXPECT_EQ(creatureTwo, world->getCreature(creatureTwo.getCreatureId()));
  EXPECT_EQ(creaturePositionTwo, world->getCreaturePosition(creatureTwo.getCreatureId()));
}

TEST_F(WorldTest, RemoveCreature)
{
  // Add first Creature
  Creature creatureOne("TestCreatureOne");
  MockCreatureCtrl creatureCtrlOne;
  Position creaturePositionOne(192, 192, 7);

  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);

  // Add second Creature
  Creature creatureTwo("TestCreatureTwo");
  MockCreatureCtrl creatureCtrlTwo;
  Position creaturePositionTwo(193, 192, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(creatureTwo, creaturePositionTwo)).Times(1);
  world->addCreature(&creatureTwo, &creatureCtrlTwo, creaturePositionTwo);

  // Remove first Creature
  // Removing creatureOne should call creatureTwo's CreatureCtrl::onCreatureDespawn
  EXPECT_CALL(creatureCtrlTwo, onCreatureDespawn(creatureOne, creaturePositionOne, _)).Times(1);
  world->removeCreature(creatureOne.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureOne.getCreatureId()));

  // Remove second Creature
  // Should not call creatureTwo's CreatureCtrl::onCreatureDespawn
  EXPECT_CALL(creatureCtrlOne, onCreatureDespawn(_, _, _)).Times(0);
  world->removeCreature(creatureTwo.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureTwo.getCreatureId()));
}
