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

#include <memory>
#include <unordered_map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mocks/itemfactory_mock.h"
#include "mocks/creaturectrl_mock.h"
#include "world.h"
#include "creature.h"
#include "creature_ctrl.h"
#include "position.h"
#include "item.h"

using ::testing::AtLeast;
using ::testing::_;

class WorldTest : public ::testing::Test
{
 protected:
  WorldTest()
  {

    // We need to build a small simple map, currently with invalid ground items
    // TODO(gurka): MockItem
    // Valid positions are (192, 192, 7) to (207, 207, 7)
    std::unordered_map<Position, Tile, Position::Hash> tiles;
    for (auto x = 0; x < 16; x++)
    {
      for (auto y = 0; y < 16; y++)
      {
        // TODO(gurka): This damn 192 constant again...
        tiles.emplace(std::piecewise_construct,
                      std::forward_as_tuple(Position(192 + x, 192 + y, 7)),
                      std::forward_as_tuple(Item()));
      }
    }

    world = std::make_unique<World>(std::make_unique<MockItemFactory>(), 16, 16, std::move(tiles));
  }

  std::unique_ptr<World> world;
};

TEST_F(WorldTest, AddCreature)
{
  // Add first Creature at (192, 192, 7)
  Creature creatureOne("TestCreatureOne");
  MockCreatureCtrl creatureCtrlOne;
  Position creaturePositionOne(192, 192, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, creatureOne, creaturePositionOne));
  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);

  EXPECT_TRUE(world->creatureExists(creatureOne.getCreatureId()));
  EXPECT_EQ(creatureOne, world->getCreature(creatureOne.getCreatureId()));
  EXPECT_EQ(creaturePositionOne, world->getCreaturePosition(creatureOne.getCreatureId()));


  // Add second Creature at (193, 193, 7)
  Creature creatureTwo("TestCreatureTwo");
  MockCreatureCtrl creatureCtrlTwo;
  Position creaturePositionTwo(193, 193, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, creatureTwo, creaturePositionTwo));
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(_, creatureTwo, creaturePositionTwo));
  world->addCreature(&creatureTwo, &creatureCtrlTwo, creaturePositionTwo);

  EXPECT_TRUE(world->creatureExists(creatureTwo.getCreatureId()));
  EXPECT_EQ(creatureTwo, world->getCreature(creatureTwo.getCreatureId()));
  EXPECT_EQ(creaturePositionTwo, world->getCreaturePosition(creatureTwo.getCreatureId()));


  // Add third Creature at (202, 193, 7)
  // Should not call creatureOne's onCreatureSpawn due to being outside its vision (on x axis)
  Creature creatureThree("TestCreatureThree");
  MockCreatureCtrl creatureCtrlThree;
  Position creaturePositionThree(202, 193, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _, _)).Times(0);
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(_, creatureThree, creaturePositionThree));
  EXPECT_CALL(creatureCtrlThree, onCreatureSpawn(_, creatureThree, creaturePositionThree));
  world->addCreature(&creatureThree, &creatureCtrlThree, creaturePositionThree);

  EXPECT_TRUE(world->creatureExists(creatureThree.getCreatureId()));
  EXPECT_EQ(creatureThree, world->getCreature(creatureThree.getCreatureId()));
  EXPECT_EQ(creaturePositionThree, world->getCreaturePosition(creatureThree.getCreatureId()));


  // Add fourth Creature at (195, 200, 7)
  // Should not call creatureOne's onCreatureSpawn due to being outside its vision (on y axis)
  Creature creatureFour("TestCreatureFour");
  MockCreatureCtrl creatureCtrlFour;
  Position creaturePositionFour(195, 200, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _, _)).Times(0);
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(_, creatureFour, creaturePositionFour));
  EXPECT_CALL(creatureCtrlThree, onCreatureSpawn(_, creatureFour, creaturePositionFour));
  EXPECT_CALL(creatureCtrlFour, onCreatureSpawn(_, creatureFour, creaturePositionFour));
  world->addCreature(&creatureFour, &creatureCtrlFour, creaturePositionFour);

  EXPECT_TRUE(world->creatureExists(creatureFour.getCreatureId()));
  EXPECT_EQ(creatureFour, world->getCreature(creatureFour.getCreatureId()));
  EXPECT_EQ(creaturePositionFour, world->getCreaturePosition(creatureFour.getCreatureId()));
}

TEST_F(WorldTest, RemoveCreature)
{
  // Add same Creatures as in AddCreature-test with same positions, i.e:
  // creatureOne can only see creatureTwo
  // creatureTwo can see all Creatures
  // creatureThree can only see creatureTwo and creatureFour
  // creatureFour can only see creatureTwo and creatureThree

  Creature creatureOne("TestCreatureOne");
  Creature creatureTwo("TestCreatureTwo");
  Creature creatureThree("TestCreatureThree");
  Creature creatureFour("TestCreatureFour");

  MockCreatureCtrl creatureCtrlOne;
  MockCreatureCtrl creatureCtrlTwo;
  MockCreatureCtrl creatureCtrlThree;
  MockCreatureCtrl creatureCtrlFour;

  Position creaturePositionOne(192, 192, 7);
  Position creaturePositionTwo(193, 193, 7);
  Position creaturePositionThree(202, 193, 7);
  Position creaturePositionFour(195, 200, 7);

  // We don't actually care about these since they are tested in AddCreature
  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _, _)).Times(AtLeast(0));
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(_, _, _)).Times(AtLeast(0));
  EXPECT_CALL(creatureCtrlThree, onCreatureSpawn(_, _, _)).Times(AtLeast(0));
  EXPECT_CALL(creatureCtrlFour, onCreatureSpawn(_, _, _)).Times(AtLeast(0));

  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);
  world->addCreature(&creatureTwo, &creatureCtrlTwo, creaturePositionTwo);
  world->addCreature(&creatureThree, &creatureCtrlThree, creaturePositionThree);
  world->addCreature(&creatureFour, &creatureCtrlFour, creaturePositionFour);

  // Remove creatureOne
  EXPECT_CALL(creatureCtrlOne, onCreatureDespawn(_, creatureOne, creaturePositionOne, _));
  EXPECT_CALL(creatureCtrlTwo, onCreatureDespawn(_, creatureOne, creaturePositionOne, _));
  EXPECT_CALL(creatureCtrlThree, onCreatureDespawn(_, _, _, _)).Times(0);
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(_, _, _, _)).Times(0);
  world->removeCreature(creatureOne.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureOne.getCreatureId()));

  // Remove creatureTwo
  EXPECT_CALL(creatureCtrlTwo, onCreatureDespawn(_, creatureTwo, creaturePositionTwo, _));
  EXPECT_CALL(creatureCtrlThree, onCreatureDespawn(_, creatureTwo, creaturePositionTwo, _));
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(_, creatureTwo, creaturePositionTwo, _));
  world->removeCreature(creatureTwo.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureTwo.getCreatureId()));

  // Remove creatureThree
  EXPECT_CALL(creatureCtrlThree, onCreatureDespawn(_, creatureThree, creaturePositionThree, _));
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(_, creatureThree, creaturePositionThree, _));
  world->removeCreature(creatureThree.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureThree.getCreatureId()));

  // Remove creatureFour
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(_, creatureFour, creaturePositionFour, _));
  world->removeCreature(creatureFour.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureFour.getCreatureId()));
}

TEST_F(WorldTest, CreatureMoveSingleCreature)
{
  // TODO(gurka): Fix Item in test environment:

  /*
Program received signal SIGSEGV, Segmentation fault.
0x00000000005965c8 in Item::isBlocking (this=0x7f8ff0) at /home/simon/code/gameserver/gameserver/world/export/item.h:80
80        bool isBlocking() const { return itemData_->isBlocking; }
(gdb) print itemData_
$1 = (const ItemData *) 0x0
  */

  /*
  Creature creatureOne("TestCreatureOne");
  MockCreatureCtrl creatureCtrlOne;
  Position creaturePositionOne(192, 192, 7);
  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _, _));
  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);

  // Test with Direction
  Direction direction(Direction::EAST);
  world->creatureMove(creatureOne.getCreatureId(), direction);
  EXPECT_EQ(creaturePositionOne.addDirection(direction), world->getCreaturePosition(creatureOne.getCreatureId()));

  // Test with Position, from (193, 192, 7) to (193, 193, 7)
  Position position(193, 193, 7);
  world->creatureMove(creatureOne.getCreatureId(), position);
  EXPECT_EQ(position, world->getCreaturePosition(creatureOne.getCreatureId()));
  */
}
