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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "container_manager.h"
#include "player_ctrl_mock.h"
#include "world_interface.h"

using ::testing::Return;

class ContainerManagerTest : public ::testing::Test
{
 protected:
  ContainerManager container_manager_;

  const CreatureId player_id = 12345;

  // Made up item ids
  const ItemId backpack_item_id = 1;
  const ItemId stone_item_id = 2;
};

TEST_F(ContainerManagerTest, containerCreation)
{
  PlayerCtrlMock player_ctrl_mock;
  const auto backpack_item = Item(backpack_item_id);

  // Inform ContainerManager about this player
  EXPECT_CALL(player_ctrl_mock, getPlayerId()).WillOnce(Return(player_id));
  container_manager_.playerSpawn(&player_ctrl_mock);

  // Create a container located in player inventory slot 0
  const auto containerId = container_manager_.createContainer(&player_ctrl_mock,
                                                              backpack_item.getItemId(),
                                                              ItemPosition(GamePosition(0),
                                                                           backpack_item.getItemId(),
                                                                           0));
  // containerId should start at 64
  EXPECT_EQ(64, containerId);

  // Verify first container's attributes
  const auto* container = container_manager_.getContainer(containerId);
  EXPECT_EQ(containerId,               container->id);
  EXPECT_EQ(0,                         container->weight);  // not yet implemented
  EXPECT_EQ(backpack_item.getItemId(), container->itemId);
  EXPECT_EQ(Container::INVALID_ID,     container->parentContainerId);
  EXPECT_TRUE(container->rootItemPosition.getGamePosition().isInventory());
  EXPECT_EQ(0,                         container->rootItemPosition.getGamePosition().getInventorySlot());
  EXPECT_EQ(backpack_item.getItemId(), container->rootItemPosition.getItemId());
  EXPECT_EQ(0,                         container->rootItemPosition.getStackPosition());

  // Due to temporary add of items in container_manager.cc
  //EXPECT_TRUE(container->items.empty());
  EXPECT_EQ(3u, container->items.size());

  // Create a new container inside the first container
  const auto containerSlot = 4;
  const auto containerId2 = container_manager_.createContainer(&player_ctrl_mock,
                                                               backpack_item.getItemId(),
                                                               ItemPosition(GamePosition(containerId, containerSlot),
                                                                            backpack_item.getItemId(),
                                                                            0));

  // Container id should increase by one
  EXPECT_EQ(containerId + 1, containerId2);

  // Verify second container's attributes
  const auto* container2 = container_manager_.getContainer(containerId2);
  EXPECT_EQ(containerId2,              container2->id);
  EXPECT_EQ(0,                         container2->weight);  // not yet implemented
  EXPECT_EQ(backpack_item.getItemId(), container2->itemId);
  EXPECT_EQ(containerId,               container2->parentContainerId);
  EXPECT_TRUE(container2->rootItemPosition.getGamePosition().isInventory());
  EXPECT_EQ(0,                         container2->rootItemPosition.getGamePosition().getInventorySlot());
  EXPECT_EQ(backpack_item.getItemId(), container2->rootItemPosition.getItemId());
  EXPECT_EQ(0,                         container2->rootItemPosition.getStackPosition());
}
