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
#include "item.h"

#include "gtest/gtest.h"

class TileTest : public ::testing::Test
{
 public:
  TileTest()
  {
    ItemData itemDataA;
    itemDataA.valid = true;
    itemDataA.name = "Item A";
    Item::setItemData(itemIdA, itemDataA);

    ItemData itemDataB;
    itemDataB.valid = true;
    itemDataB.name = "Item B";
    Item::setItemData(itemIdB, itemDataB);

    ItemData itemDataC;
    itemDataC.valid = true;
    itemDataC.name = "Item C";
    Item::setItemData(itemIdC, itemDataC);

    ItemData itemDataD;
    itemDataD.valid = true;
    itemDataD.name = "Item D";
    Item::setItemData(itemIdD, itemDataD);
  }

 protected:
  static constexpr ItemId itemIdA = 1;
  static constexpr ItemId itemIdB = 2;
  static constexpr ItemId itemIdC = 3;
  static constexpr ItemId itemIdD = 4;
};

TEST_F(TileTest, Constructor)
{
  Item groundItem(itemIdA);
  Tile tileA(groundItem);

  ASSERT_EQ(tileA.getItem(0).getItemId(), groundItem.getItemId());
  ASSERT_EQ(tileA.getNumberOfThings(), 1u);  // Only ground item
}

TEST_F(TileTest, AddRemoveCreatures)
{
  Item groundItem(itemIdA);
  Tile tile(groundItem);
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
  Item groundItem(itemIdD);
  Tile tile(groundItem);

  Item itemA(itemIdA);
  Item itemB(itemIdB);
  Item itemC(itemIdC);

  // Add an item and remove it
  tile.addItem(itemA);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 1u);  // Ground item + item

  auto result = tile.removeItem(itemA.getItemId(), 1);  // Only item => stackpos = 1
  ASSERT_TRUE(result);
  ASSERT_EQ(tile.getNumberOfThings(), 1u + 0u);

  // Add all three items
  tile.addItem(itemA);
  tile.addItem(itemB);
  tile.addItem(itemC);
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
