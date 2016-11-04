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

#include "item.h"

#include "gtest/gtest.h"

class ItemTest : public ::testing::Test
{
 public:
  ItemTest()
  {
    dummyItemA_.id = 1;
    dummyItemA_.name = "DummyItemA";

    dummyItemB_.id = 2;
    dummyItemB_.name = "DummyItemB";
    dummyItemB_.attributes.insert(std::make_pair("string", "test"));
    dummyItemB_.attributes.insert(std::make_pair("integer", "1234"));
    dummyItemB_.attributes.insert(std::make_pair("float", "3.14"));
  }

  ItemData dummyItemA_;
  ItemData dummyItemB_;
};

static ItemData dummyItemA;

TEST_F(ItemTest, Constructor)
{
  Item invalidItem(nullptr);
  ASSERT_FALSE(invalidItem.isValid());

  Item validItem(&dummyItemA_);
  ASSERT_TRUE(validItem.isValid());
}

TEST_F(ItemTest, Attribute)
{
  Item testItem(&dummyItemB_);
  ASSERT_TRUE(testItem.isValid());

  ASSERT_EQ(testItem.getAttribute<std::string>("string"), "test");
  ASSERT_EQ(testItem.getAttribute<int>("integer"), 1234);
  ASSERT_FLOAT_EQ(testItem.getAttribute<float>("float"), 3.14f);
}

TEST_F(ItemTest, Equals)
{
  Item testItemA(&dummyItemA_);
  Item testItemAA(&dummyItemA_);
  Item testItemB(&dummyItemB_);
  Item testItemC(nullptr);

  ASSERT_TRUE(testItemA.isValid());
  ASSERT_TRUE(testItemB.isValid());
  ASSERT_FALSE(testItemC.isValid());

  ASSERT_EQ(testItemA, testItemAA);
  ASSERT_NE(testItemA, testItemB);
  ASSERT_NE(testItemA, testItemC);

  Item testItemD = testItemA;
  ASSERT_EQ(testItemA, testItemD);

  Item testItemE(testItemB);
  ASSERT_EQ(testItemB, testItemE);

  ASSERT_EQ(testItemC, Item(nullptr));
}
