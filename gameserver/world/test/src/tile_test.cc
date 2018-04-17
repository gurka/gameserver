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

#include "tile.h"
#include "item_mock.h"

#include "gtest/gtest.h"

using ::testing::ReturnRef;

class TileTest : public ::testing::Test
{
};

TEST_F(TileTest, Constructor)
{
  ItemType groundItemType;
  groundItemType.id = 123;

  ItemMock groundItem;
  const auto tile = Tile(&groundItem);

  EXPECT_CALL(groundItem, getItemType()).WillOnce(ReturnRef(groundItemType));
  ASSERT_EQ(123, tile.getItem(0)->getItemType().id);
  ASSERT_EQ(1u, tile.getNumberOfThings());  // Only ground item
}

TEST_F(TileTest, AddRemoveCreatures)
{
  ItemType groundItemType;
  groundItemType.id = 123;

  ItemMock groundItem;
  auto tile = Tile(&groundItem);

  CreatureId creatureA(1);
  CreatureId creatureB(2);
  CreatureId creatureC(3);

  // Add a creature and remove it
  tile.addCreature(creatureA);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);  // Ground item + creature

  auto result = tile.removeCreature(creatureA);
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);

  // Add all three creatures
  tile.addCreature(creatureA);
  tile.addCreature(creatureB);
  tile.addCreature(creatureC);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 3u);

  // Remove creatureA and creatureC
  result = tile.removeCreature(creatureA);
  ASSERT_TRUE(result);
  result = tile.removeCreature(creatureC);
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);

  // Try to remove creatureA again
  result = tile.removeCreature(creatureA);
  ASSERT_FALSE(result);

  // Remove last creature
  result = tile.removeCreature(creatureB);
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);
}

TEST_F(TileTest, AddRemoveItems)
{
  ItemType groundItemType;
  groundItemType.id = 123;

  ItemMock groundItem;
  auto tile = Tile(&groundItem);

  ItemMock itemA;
  ItemMock itemB;
  ItemMock itemC;

  // Add an item and remove it
  tile.addItem(&itemA);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);  // Ground item + item

  auto result = tile.removeItem(itemA.getItemId(), 1);  // Only item => stackpos = 1
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);

  // Add all three items
  tile.addItem(&itemA);
  tile.addItem(&itemB);
  tile.addItem(&itemC);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 3u);

  // Remove itemA and itemC
  result = tile.removeItem(itemA.getItemId(), 3);  // Two items were added after itemA => stackpos = 3
  ASSERT_TRUE(result);
  result = tile.removeItem(itemC.getItemId(), 1);  // itemC was added last => stackpos = 1
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);

  // Try to remove itemA again
  result = tile.removeItem(itemA.getItemId(), 1);
  ASSERT_FALSE(result);

  // Remove last item
  result = tile.removeItem(itemB.getItemId(), 1);  // Only item => stackpos = 1
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);
}
