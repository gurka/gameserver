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

#include <memory>
#include <unordered_map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "creaturectrl_mock.h"
#include "item_mock.h"
#include "world.h"
#include "creature.h"
#include "creature_ctrl.h"
#include "position.h"
#include "item.h"

namespace world
{

using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::_;

class WorldTest : public ::testing::Test
{
 protected:
  WorldTest()
  {

    // We need to build a small simple map
    // Valid positions are (192, 192, 7) to (207, 207, 7)
    std::vector<Tile> tiles;
    for (auto x = 0; x < 16; x++)
    {
      for (auto y = 0; y < 16; y++)
      {
        tiles.emplace_back(&itemMock_);
      }
    }

    // Have all ground items be non-blocking
    itemType_.is_ground = true;
    itemType_.speed = 0;
    itemType_.is_blocking = false;
    EXPECT_CALL(itemMock_, getItemType()).WillRepeatedly(ReturnRef(itemType_));

    world = std::make_unique<World>(16, 16, std::move(tiles));
    cworld = world.get();
  }

  ItemMock itemMock_;
  common::ItemType itemType_;
  std::unique_ptr<World> world;
  const World* cworld;
};

TEST_F(WorldTest, AddCreature)
{
  // Add first Creature at (192, 192, 7)
  // Can see from (184, 186, 7) to (201, 199, 7)
  common::Creature creatureOne(1U, "TestCreatureOne");
  MockCreatureCtrl creatureCtrlOne;
  common::Position creaturePositionOne(192, 192, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(creatureOne, creaturePositionOne));
  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);

  EXPECT_TRUE(world->creatureExists(creatureOne.getCreatureId()));
  //EXPECT_EQ(&creatureOne, world->getCreature(creatureOne.getCreatureId()));
  EXPECT_EQ(creaturePositionOne, *(cworld->getCreaturePosition(creatureOne.getCreatureId())));


  // Add second Creature at (193, 193, 7)
  // Can see from (185, 187, 7) to (202, 200, 7)
  common::Creature creatureTwo(2U, "TestCreatureTwo");
  MockCreatureCtrl creatureCtrlTwo;
  common::Position creaturePositionTwo(193, 193, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(creatureTwo, creaturePositionTwo));
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(creatureTwo, creaturePositionTwo));
  world->addCreature(&creatureTwo, &creatureCtrlTwo, creaturePositionTwo);

  EXPECT_TRUE(world->creatureExists(creatureTwo.getCreatureId()));
  //EXPECT_EQ(creatureTwo, cworld->getCreature(creatureTwo.getCreatureId()));
  EXPECT_EQ(creaturePositionTwo, *(cworld->getCreaturePosition(creatureTwo.getCreatureId())));


  // Add third Creature at (202, 193, 7)
  // Can see from (194, 187, 7) to (211, 200, 7)
  // Should not call creatureOne's onCreatureSpawn due to being outside its vision (on x axis)
  common::Creature creatureThree(3U, "TestCreatureThree");
  MockCreatureCtrl creatureCtrlThree;
  common::Position creaturePositionThree(202, 193, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _)).Times(0);
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(creatureThree, creaturePositionThree));
  EXPECT_CALL(creatureCtrlThree, onCreatureSpawn(creatureThree, creaturePositionThree));
  world->addCreature(&creatureThree, &creatureCtrlThree, creaturePositionThree);

  EXPECT_TRUE(world->creatureExists(creatureThree.getCreatureId()));
  //EXPECT_EQ(creatureThree, cworld->getCreature(creatureThree.getCreatureId()));
  EXPECT_EQ(creaturePositionThree, *(cworld->getCreaturePosition(creatureThree.getCreatureId())));


  // Add fourth Creature at (195, 200, 7)
  // Can see from (187, 194, 7) to (204, 207, 7)
  // Should not call creatureOne's onCreatureSpawn due to being outside its vision (on y axis)
  common::Creature creatureFour(4U, "TestCreatureFour");
  MockCreatureCtrl creatureCtrlFour;
  common::Position creaturePositionFour(195, 200, 7);

  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _)).Times(0);
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(creatureFour, creaturePositionFour));
  EXPECT_CALL(creatureCtrlThree, onCreatureSpawn(creatureFour, creaturePositionFour));
  EXPECT_CALL(creatureCtrlFour, onCreatureSpawn(creatureFour, creaturePositionFour));
  world->addCreature(&creatureFour, &creatureCtrlFour, creaturePositionFour);

  EXPECT_TRUE(world->creatureExists(creatureFour.getCreatureId()));
  //EXPECT_EQ(creatureFour, cworld->getCreature(creatureFour.getCreatureId()));
  EXPECT_EQ(creaturePositionFour, *(cworld->getCreaturePosition(creatureFour.getCreatureId())));
}

TEST_F(WorldTest, RemoveCreature)
{
  // Add same Creatures as in AddCreature-test with same positions, i.e:
  // creatureOne can only see creatureTwo
  // creatureTwo can see everybody
  // creatureThree can only see creatureFour
  // creatureFour cannot see anyone

  common::Creature creatureOne(1U, "TestCreatureOne");
  common::Creature creatureTwo(2U, "TestCreatureTwo");
  common::Creature creatureThree(3U, "TestCreatureThree");
  common::Creature creatureFour(4U, "TestCreatureFour");

  MockCreatureCtrl creatureCtrlOne;
  MockCreatureCtrl creatureCtrlTwo;
  MockCreatureCtrl creatureCtrlThree;
  MockCreatureCtrl creatureCtrlFour;

  common::Position creaturePositionOne(192, 192, 7);
  common::Position creaturePositionTwo(193, 193, 7);
  common::Position creaturePositionThree(202, 193, 7);
  common::Position creaturePositionFour(195, 200, 7);

  // We don't actually care about these since they are tested in AddCreature
  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _)).Times(2);    // himself and creatureTwo
  EXPECT_CALL(creatureCtrlTwo, onCreatureSpawn(_, _)).Times(3);    // himself, creatureThree and creatureFour
  EXPECT_CALL(creatureCtrlThree, onCreatureSpawn( _, _)).Times(2);  // himself and creatureFour
  EXPECT_CALL(creatureCtrlFour, onCreatureSpawn(_, _)).Times(1);   // only himself

  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);
  world->addCreature(&creatureTwo, &creatureCtrlTwo, creaturePositionTwo);
  world->addCreature(&creatureThree, &creatureCtrlThree, creaturePositionThree);
  world->addCreature(&creatureFour, &creatureCtrlFour, creaturePositionFour);

  // Remove creatureOne
  EXPECT_CALL(creatureCtrlOne, onCreatureDespawn(creatureOne, creaturePositionOne, _));
  EXPECT_CALL(creatureCtrlTwo, onCreatureDespawn(creatureOne, creaturePositionOne, _));
  EXPECT_CALL(creatureCtrlThree, onCreatureDespawn(_, _, _)).Times(0);
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(_, _, _)).Times(0);
  world->removeCreature(creatureOne.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureOne.getCreatureId()));

  // Remove creatureTwo
  EXPECT_CALL(creatureCtrlTwo, onCreatureDespawn(creatureTwo, creaturePositionTwo, _));
  EXPECT_CALL(creatureCtrlThree, onCreatureDespawn(_, _, _)).Times(0);
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(_, _, _)).Times(0);
  world->removeCreature(creatureTwo.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureTwo.getCreatureId()));

  // Remove creatureThree
  EXPECT_CALL(creatureCtrlThree, onCreatureDespawn(creatureThree, creaturePositionThree, _));
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(_, _, _)).Times(0);
  world->removeCreature(creatureThree.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureThree.getCreatureId()));

  // Remove creatureFour
  EXPECT_CALL(creatureCtrlFour, onCreatureDespawn(creatureFour, creaturePositionFour, _));
  world->removeCreature(creatureFour.getCreatureId());
  EXPECT_FALSE(world->creatureExists(creatureFour.getCreatureId()));
}

TEST_F(WorldTest, CreatureMoveSingleCreature)
{
  common::Creature creatureOne(1U, "TestCreatureOne");
  MockCreatureCtrl creatureCtrlOne;
  common::Position creaturePositionOne(192, 192, 7);
  EXPECT_CALL(creatureCtrlOne, onCreatureSpawn(_, _));
  world->addCreature(&creatureOne, &creatureCtrlOne, creaturePositionOne);

  // Test with Direction
  EXPECT_CALL(creatureCtrlOne, onCreatureMove(_, _, _, _));
  common::Direction direction(common::Direction::EAST);
  world->creatureMove(creatureOne.getCreatureId(), direction);
  EXPECT_EQ(creaturePositionOne.addDirection(direction),
            *(cworld->getCreaturePosition(creatureOne.getCreatureId())));

  // Test with Position, from (193, 192, 7) to (193, 193, 7)
  EXPECT_CALL(creatureCtrlOne, onCreatureMove(_, _, _, _));
  common::Position position(193, 193, 7);
  world->creatureMove(creatureOne.getCreatureId(), position);
  EXPECT_EQ(position, *(cworld->getCreaturePosition(creatureOne.getCreatureId())));
}

}  // namespace world
