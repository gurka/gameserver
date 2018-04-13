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
using ::testing::Ref;
using ::testing::_;

class ContainerManagerTest : public ::testing::Test
{
 protected:
  ContainerManagerTest()
  {
    ItemData item_data;
    item_data.valid = true;
    item_data.isContainer = true;
    Item::setItemData(backpack_item_id, item_data);

    EXPECT_CALL(player_ctrl_mock_, getPlayerId()).WillRepeatedly(Return(player_id));

    // Inform ContainerManager about this player
    container_manager_.playerSpawn(&player_ctrl_mock_);
  }

  ~ContainerManagerTest()
  {
    container_manager_.playerDespawn(&player_ctrl_mock_);
  }

  PlayerCtrlMock player_ctrl_mock_;
  ContainerManager container_manager_;
  const CreatureId player_id = 12345;
  const ItemId backpack_item_id = 1;
};

TEST_F(ContainerManagerTest, containerCreation)
{
  const auto backpack_item = Item(backpack_item_id);

  // Create a container located in player inventory slot 0
  const auto containerId = container_manager_.createContainer(&player_ctrl_mock_,
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
  const auto containerId2 = container_manager_.createContainer(&player_ctrl_mock_,
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

TEST_F(ContainerManagerTest, useContainer)
{
  // A backpack located in player inventory slot 0
  auto item1 = Item(backpack_item_id);
  const auto item_position1 = ItemPosition(GamePosition(0), item1.getItemId(), 0);
  item1.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                          item1.getItemId(),
                                                          item_position1));
  const auto* container1 = container_manager_.getContainer(item1.getContainerId());

  // A backpack located in player inventory slot 1
  auto item2 = Item(backpack_item_id);
  const auto item_position2 = ItemPosition(GamePosition(1), item2.getItemId(), 0);
  item2.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                          item2.getItemId(),
                                                          item_position2));
  const auto* container2 = container_manager_.getContainer(item2.getContainerId());

  // clientContainerIds to use
  const auto clientContainerId1 = 14;
  const auto clientContainerId2 = 25;
  const auto clientContainerId3 = 39;

  // Using item1 should open container1
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container1), Ref(item1)));
  container_manager_.useContainer(&player_ctrl_mock_, item1, item_position1, clientContainerId1);

  // Container1 is open, container2 is closed
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) != nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) == nullptr);

  // Using item2 (with a new clientContainerId) should open container2
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId2, Ref(*container2), Ref(item2)));
  container_manager_.useContainer(&player_ctrl_mock_, item2, item_position2, clientContainerId2);

  // Container1 and container2 are open
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) != nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);

  // Using item1 should close container1 (closing a container needs ack)
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(clientContainerId1));
  container_manager_.useContainer(&player_ctrl_mock_, item1, item_position1, clientContainerId3);
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(clientContainerId1));
  container_manager_.closeContainer(&player_ctrl_mock_, clientContainerId1);

  // Container1 is closed, container2 is open
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) == nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);

  // Using item1 with clientContainerId2 should open container1 with that clientContainerId
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId2, Ref(*container1), Ref(item1)));
  container_manager_.useContainer(&player_ctrl_mock_, item1, item_position1, clientContainerId2);

  // Container1 is open but with clientContainerId2, clientContainer1 is closed
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) == nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);
}

TEST_F(ContainerManagerTest, parentContainer)
{
  // A backpack located in player inventory slot 0
  auto item1 = Item(backpack_item_id);
  const auto item_position1 = ItemPosition(GamePosition(0), item1.getItemId(), 0);
  item1.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                          item1.getItemId(),
                                                          item_position1));
  const auto* container1 = container_manager_.getContainer(item1.getContainerId());

  // A backpack located inside the first backpack, slot 0
  auto item2 = Item(backpack_item_id);
  const auto item_position2 = ItemPosition(GamePosition(item1.getContainerId(), 0), item2.getItemId(), 0);
  item2.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                          item2.getItemId(),
                                                          item_position2));
  const auto* container2 = container_manager_.getContainer(item2.getContainerId());

  const auto clientContainerId1 = 9;

  // Open first container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container1), Ref(item1)));
  container_manager_.useContainer(&player_ctrl_mock_, item1, item_position1, clientContainerId1);

  // Open second container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container2), Ref(item2)));
  container_manager_.useContainer(&player_ctrl_mock_, item2, item_position2, clientContainerId1);

  // Open second container's parent (i.e. the first container)
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container1), _));  // TODO: Item matcher
  container_manager_.openParentContainer(&player_ctrl_mock_, clientContainerId1);

  // Re-open the second container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container2), Ref(item2)));
  container_manager_.useContainer(&player_ctrl_mock_, item2, item_position2, clientContainerId1);

  // Open first container with new clientContainerId
  const auto clientContainerId2 = 33;
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId2, Ref(*container1), Ref(item1)));
  container_manager_.useContainer(&player_ctrl_mock_, item1, item_position1, clientContainerId2);

  // Both containers should now be open
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) != nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);

  // Open second container's parent, container1 should now be open twice
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container1), _));
  container_manager_.openParentContainer(&player_ctrl_mock_, clientContainerId1);

  ASSERT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) != nullptr);
  ASSERT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);
  EXPECT_EQ(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1)->id,
            container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2)->id);
}
