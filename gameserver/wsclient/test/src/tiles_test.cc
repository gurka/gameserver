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

#include "../../src/tiles.h"

#include "gtest/gtest.h"

namespace wsclient
{

// Tests are based on doc/world.txt examples

TEST(TilesTest, positionIsKnown)
{
  wsworld::Tiles tiles;

  // Test above sea level
  tiles.setMapPosition({ 100U, 200U, 4U });

  // Test top-left corner tiles
  ASSERT_TRUE(tiles.positionIsKnown({ 89U, 191U, 7U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 90U, 192U, 6U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 91U, 193U, 5U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 92U, 194U, 4U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 93U, 195U, 3U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 94U, 196U, 2U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 95U, 197U, 1U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 96U, 198U, 0U }));

  // Test bottom-right corner tiles
  ASSERT_TRUE(tiles.positionIsKnown({ 106U, 204U, 7U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 107U, 205U, 6U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 108U, 206U, 5U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 109U, 207U, 4U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 110U, 208U, 3U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 111U, 209U, 2U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 112U, 210U, 1U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 113U, 211U, 0U }));

  // Test a few outside
  ASSERT_FALSE(tiles.positionIsKnown({ 91U, 200U, 4U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 100U, 208U, 4U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 88U, 200U, 7U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 100U, 205U, 7U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 94U, 205U, 0U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 98U, 212U, 0U }));

  // Test below sea level
  tiles.setMapPosition({ 10U, 10U, 14U });

  // Test top-left corner tiles
  ASSERT_TRUE(tiles.positionIsKnown({ 4U, 6U, 12U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 3U, 5U, 13U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 2U, 4U, 14U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 1U, 3U, 15U }));

  // Test bottom-right corner tiles
  ASSERT_TRUE(tiles.positionIsKnown({ 21U, 19U, 12U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 20U, 18U, 13U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 19U, 17U, 14U }));
  ASSERT_TRUE(tiles.positionIsKnown({ 18U, 16U, 15U }));

  // Test a few outside
  ASSERT_FALSE(tiles.positionIsKnown({ 2U, 13U, 13U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 8U, 19U, 13U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 1U, 10U, 14U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 10U, 18U, 14U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 10U, 10U, 10U }));
  ASSERT_FALSE(tiles.positionIsKnown({ 10U, 10U, 5U }));
}

TEST(TilesTest, globalToLocalPosition)
{
  wsworld::Tiles tiles;

  // Test above sea level
  tiles.setMapPosition({ 100U, 200U, 4U });

  ASSERT_EQ(common::Position(0U, 0U, 0U), tiles.globalToLocalPosition({ 89U, 191U, 7U }));
  ASSERT_EQ(common::Position(0U, 0U, 1U), tiles.globalToLocalPosition({ 90U, 192U, 6U }));
  ASSERT_EQ(common::Position(0U, 0U, 2U), tiles.globalToLocalPosition({ 91U, 193U, 5U }));
  ASSERT_EQ(common::Position(0U, 0U, 3U), tiles.globalToLocalPosition({ 92U, 194U, 4U }));
  ASSERT_EQ(common::Position(0U, 0U, 4U), tiles.globalToLocalPosition({ 93U, 195U, 3U }));
  ASSERT_EQ(common::Position(0U, 0U, 5U), tiles.globalToLocalPosition({ 94U, 196U, 2U }));
  ASSERT_EQ(common::Position(0U, 0U, 6U), tiles.globalToLocalPosition({ 95U, 197U, 1U }));
  ASSERT_EQ(common::Position(0U, 0U, 7U), tiles.globalToLocalPosition({ 96U, 198U, 0U }));

  ASSERT_EQ(common::Position(17U, 13U, 0U), tiles.globalToLocalPosition({ 106U, 204U, 7U }));
  ASSERT_EQ(common::Position(17U, 13U, 1U), tiles.globalToLocalPosition({ 107U, 205U, 6U }));
  ASSERT_EQ(common::Position(17U, 13U, 2U), tiles.globalToLocalPosition({ 108U, 206U, 5U }));
  ASSERT_EQ(common::Position(17U, 13U, 3U), tiles.globalToLocalPosition({ 109U, 207U, 4U }));
  ASSERT_EQ(common::Position(17U, 13U, 4U), tiles.globalToLocalPosition({ 110U, 208U, 3U }));
  ASSERT_EQ(common::Position(17U, 13U, 5U), tiles.globalToLocalPosition({ 111U, 209U, 2U }));
  ASSERT_EQ(common::Position(17U, 13U, 6U), tiles.globalToLocalPosition({ 112U, 210U, 1U }));
  ASSERT_EQ(common::Position(17U, 13U, 7U), tiles.globalToLocalPosition({ 113U, 211U, 0U }));

  // Test below sea level
  tiles.setMapPosition({ 10U, 10U, 14U });

  ASSERT_EQ(common::Position(0U, 0U, 0U), tiles.globalToLocalPosition({ 4U, 6U, 12U }));
  ASSERT_EQ(common::Position(0U, 0U, 1U), tiles.globalToLocalPosition({ 3U, 5U, 13U }));
  ASSERT_EQ(common::Position(0U, 0U, 2U), tiles.globalToLocalPosition({ 2U, 4U, 14U }));
  ASSERT_EQ(common::Position(0U, 0U, 3U), tiles.globalToLocalPosition({ 1U, 3U, 15U }));

  ASSERT_EQ(common::Position(17U, 13U, 0U), tiles.globalToLocalPosition({ 21U, 19U, 12U }));
  ASSERT_EQ(common::Position(17U, 13U, 1U), tiles.globalToLocalPosition({ 20U, 18U, 13U }));
  ASSERT_EQ(common::Position(17U, 13U, 2U), tiles.globalToLocalPosition({ 19U, 17U, 14U }));
  ASSERT_EQ(common::Position(17U, 13U, 3U), tiles.globalToLocalPosition({ 18U, 16U, 15U }));
}

TEST(TilesTest, localToGlobalPosition)
{
  wsworld::Tiles tiles;

  // Test above sea level
  tiles.setMapPosition({ 100U, 200U, 4U });

  ASSERT_EQ(common::Position(89U, 191U, 7U), tiles.localToGlobalPosition({ 0U, 0U, 0U }));
  ASSERT_EQ(common::Position(90U, 192U, 6U), tiles.localToGlobalPosition({ 0U, 0U, 1U }));
  ASSERT_EQ(common::Position(91U, 193U, 5U), tiles.localToGlobalPosition({ 0U, 0U, 2U }));
  ASSERT_EQ(common::Position(92U, 194U, 4U), tiles.localToGlobalPosition({ 0U, 0U, 3U }));
  ASSERT_EQ(common::Position(93U, 195U, 3U), tiles.localToGlobalPosition({ 0U, 0U, 4U }));
  ASSERT_EQ(common::Position(94U, 196U, 2U), tiles.localToGlobalPosition({ 0U, 0U, 5U }));
  ASSERT_EQ(common::Position(95U, 197U, 1U), tiles.localToGlobalPosition({ 0U, 0U, 6U }));
  ASSERT_EQ(common::Position(96U, 198U, 0U), tiles.localToGlobalPosition({ 0U, 0U, 7U }));

  ASSERT_EQ(common::Position(106U, 204U, 7U), tiles.localToGlobalPosition({ 17U, 13U, 0U }));
  ASSERT_EQ(common::Position(107U, 205U, 6U), tiles.localToGlobalPosition({ 17U, 13U, 1U }));
  ASSERT_EQ(common::Position(108U, 206U, 5U), tiles.localToGlobalPosition({ 17U, 13U, 2U }));
  ASSERT_EQ(common::Position(109U, 207U, 4U), tiles.localToGlobalPosition({ 17U, 13U, 3U }));
  ASSERT_EQ(common::Position(110U, 208U, 3U), tiles.localToGlobalPosition({ 17U, 13U, 4U }));
  ASSERT_EQ(common::Position(111U, 209U, 2U), tiles.localToGlobalPosition({ 17U, 13U, 5U }));
  ASSERT_EQ(common::Position(112U, 210U, 1U), tiles.localToGlobalPosition({ 17U, 13U, 6U }));
  ASSERT_EQ(common::Position(113U, 211U, 0U), tiles.localToGlobalPosition({ 17U, 13U, 7U }));

  // Test below sea level
  tiles.setMapPosition({ 10U, 10U, 14U });

  ASSERT_EQ(common::Position(4U, 6U, 12U), tiles.localToGlobalPosition({ 0U, 0U, 0U }));
  ASSERT_EQ(common::Position(3U, 5U, 13U), tiles.localToGlobalPosition({ 0U, 0U, 1U }));
  ASSERT_EQ(common::Position(2U, 4U, 14U), tiles.localToGlobalPosition({ 0U, 0U, 2U }));
  ASSERT_EQ(common::Position(1U, 3U, 15U), tiles.localToGlobalPosition({ 0U, 0U, 3U }));

  ASSERT_EQ(common::Position(21U, 19U, 12U), tiles.localToGlobalPosition({ 17U, 13U, 0U }));
  ASSERT_EQ(common::Position(20U, 18U, 13U), tiles.localToGlobalPosition({ 17U, 13U, 1U }));
  ASSERT_EQ(common::Position(19U, 17U, 14U), tiles.localToGlobalPosition({ 17U, 13U, 2U }));
  ASSERT_EQ(common::Position(18U, 16U, 15U), tiles.localToGlobalPosition({ 17U, 13U, 3U }));
}


}  // namespace wsclient
