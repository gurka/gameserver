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

#include "tile.h"
#include "item_mock.h"
#include "creature_mock.h"

#include "gtest/gtest.h"

namespace world
{

using ::testing::Return;
using ::testing::ReturnRef;

class TileTest : public ::testing::Test
{
};

TEST_F(TileTest, Constructor)
{
  common::ItemType groundItemType;
  ItemMock groundItem;
  const auto tile = Tile(&groundItem);

  EXPECT_CALL(groundItem, getItemTypeId()).WillOnce(Return(123));
  ASSERT_EQ(123, tile.getItem(0)->getItemTypeId());
  ASSERT_EQ(1u, tile.getNumberOfThings());  // Only ground item
}

TEST_F(TileTest, AddRemoveCreatures)
{
  common::ItemType groundItemType;
  ItemMock groundItem;
  auto tile = Tile(&groundItem);

  CreatureMock creatureA(1);
  CreatureMock creatureB(2);
  CreatureMock creatureC(3);

  // Add a creature and remove it
  tile.addThing(&creatureA);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);  // Ground item + creature

  auto result = tile.removeThing(1);
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);

  // Add all three creatures
  tile.addThing(&creatureA);  // stackpos 3
  tile.addThing(&creatureB);  // stackpos 2
  tile.addThing(&creatureC);  // stackpos 1
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 3u);

  // Remove creatureA and creatureC
  result = tile.removeThing(3);
  ASSERT_TRUE(result);
  result = tile.removeThing(1);
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);

  // Remove last creature
  result = tile.removeThing(1);
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);
}

TEST_F(TileTest, AddRemoveItems)
{
  common::ItemType groundItemType;
  ItemMock groundItem;
  auto tile = Tile(&groundItem);

  common::ItemType itemTypeA;
  ItemMock itemA;
  EXPECT_CALL(itemA, getItemTypeId()).WillRepeatedly(Return(1));
  EXPECT_CALL(itemA, getItemType()).WillRepeatedly(ReturnRef(itemTypeA));

  common::ItemType itemTypeB;
  ItemMock itemB;
  EXPECT_CALL(itemB, getItemTypeId()).WillRepeatedly(Return(2));
  EXPECT_CALL(itemB, getItemType()).WillRepeatedly(ReturnRef(itemTypeB));

  common::ItemType itemTypeC;
  ItemMock itemC;
  EXPECT_CALL(itemC, getItemTypeId()).WillRepeatedly(Return(3));
  EXPECT_CALL(itemC, getItemType()).WillRepeatedly(ReturnRef(itemTypeC));

  // Add an item and remove it
  tile.addThing(&itemA);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);  // Ground item + item

  auto result = tile.removeThing(1);
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);

  // Add all three items
  tile.addThing(&itemA);
  tile.addThing(&itemB);
  tile.addThing(&itemC);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 3u);

  // Remove itemA and itemC
  result = tile.removeThing(3);  // Two items were added after itemA => stackpos = 3
  ASSERT_TRUE(result);
  result = tile.removeThing(1);  // itemC was added last => stackpos = 1
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);

  // Remove last item
  result = tile.removeThing(1);  // Only item => stackpos = 1
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);
}

}  // namespace world
