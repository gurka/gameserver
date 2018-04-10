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

#include "item.h"

#include "gtest/gtest.h"

class ItemTest : public ::testing::Test
{
 public:
  ItemTest()
  {
    ItemData itemDataA;
    itemDataA.valid = true;
    itemDataA.name = "Item A";
    Item::setItemData(itemIdA, itemDataA);

    ItemData itemDataB;
    itemDataB.valid = true;
    itemDataB.name = "Item B";
    itemDataB.attributes.insert(std::make_pair("string", "test"));
    itemDataB.attributes.insert(std::make_pair("integer", "1234"));
    itemDataB.attributes.insert(std::make_pair("float", "3.14"));
    Item::setItemData(itemIdB, itemDataB);
  }

 protected:
  static constexpr ItemId itemIdA = 1;
  static constexpr ItemId itemIdB = 2;
};

TEST_F(ItemTest, Constructor)
{
  Item invalidItem{};
  ASSERT_FALSE(invalidItem.isValid());

  Item validItem(itemIdA);
  ASSERT_TRUE(validItem.isValid());
}

TEST_F(ItemTest, Attribute)
{
  Item item(itemIdB);
  ASSERT_TRUE(item.isValid());

  ASSERT_EQ(item.getAttribute<std::string>("string"), "test");
  ASSERT_EQ(item.getAttribute<int>("integer"), 1234);
  ASSERT_FLOAT_EQ(item.getAttribute<float>("float"), 3.14f);
}
