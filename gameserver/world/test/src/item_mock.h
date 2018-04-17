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

#ifndef WORLD_TEST_SRC_ITEM_MOCK_H_
#define WORLD_TEST_SRC_ITEM_MOCK_H_

#include "item.h"
#include "gmock/gmock.h"

class ItemMock : public Item
{
 public:
  MOCK_CONST_METHOD0(getItemUniqueId, ItemUniqueId());
  MOCK_CONST_METHOD0(getItemTypeId, ItemTypeId());

  MOCK_CONST_METHOD0(getItemType, const ItemType&());

  MOCK_CONST_METHOD0(getCount, int());
  MOCK_METHOD1(setCount, void(int count));
};

#endif  // WORLD_TEST_SRC_ITEM_MOCK_H_
