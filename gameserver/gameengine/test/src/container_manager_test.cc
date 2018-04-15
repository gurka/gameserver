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
    item_data.attributes.emplace_back("maxitems", "2");
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
  const CreatureId player_id = 123;
  const ItemId backpack_item_id = 456;
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
  EXPECT_TRUE(container->items.empty());

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
  EXPECT_TRUE(container2->items.empty());
}

TEST_F(ContainerManagerTest, useContainer)
{
  // Create a container located in player inventory slot 0
  auto item1 = Item(backpack_item_id);
  const auto item_position1 = ItemPosition(GamePosition(0), item1.getItemId(), 0);
  item1.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                          item1.getItemId(),
                                                          item_position1));
  const auto* container1 = container_manager_.getContainer(item1.getContainerId());

  // Create a container located in player inventory slot 1
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
  container_manager_.useContainer(&player_ctrl_mock_, item1, clientContainerId1);

  // Container1 is open, container2 is closed
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) != nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) == nullptr);

  // Using item2 (with a new clientContainerId) should open container2
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId2, Ref(*container2), Ref(item2)));
  container_manager_.useContainer(&player_ctrl_mock_, item2, clientContainerId2);

  // Container1 and container2 are open
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) != nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);

  // Using item1 should close container1 (closing a container needs ack)
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(clientContainerId1));
  container_manager_.useContainer(&player_ctrl_mock_, item1, clientContainerId3);
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(clientContainerId1));
  container_manager_.closeContainer(&player_ctrl_mock_, clientContainerId1);

  // Container1 is closed, container2 is open
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) == nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);

  // Using item1 with clientContainerId2 should open container1 with that clientContainerId
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId2, Ref(*container1), Ref(item1)));
  container_manager_.useContainer(&player_ctrl_mock_, item1, clientContainerId2);

  // Container1 is open but with clientContainerId2, clientContainer1 is closed
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId1) == nullptr);
  EXPECT_TRUE(container_manager_.getContainer(&player_ctrl_mock_, clientContainerId2) != nullptr);
}

TEST_F(ContainerManagerTest, parentContainer)
{
  // Create a container located in player inventory slot 0
  auto item1 = Item(backpack_item_id);
  const auto item_position1 = ItemPosition(GamePosition(0), item1.getItemId(), 0);
  item1.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                          item1.getItemId(),
                                                          item_position1));
  const auto* container1 = container_manager_.getContainer(item1.getContainerId());

  // Create a container located inside the first backpack, slot 0
  auto item2 = Item(backpack_item_id);
  const auto item_position2 = ItemPosition(GamePosition(item1.getContainerId(), 0), item2.getItemId(), 0);
  item2.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                          item2.getItemId(),
                                                          item_position2));
  const auto* container2 = container_manager_.getContainer(item2.getContainerId());

  const auto clientContainerId1 = 9;

  // Open first container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container1), Ref(item1)));
  container_manager_.useContainer(&player_ctrl_mock_, item1, clientContainerId1);

  // Open second container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container2), Ref(item2)));
  container_manager_.useContainer(&player_ctrl_mock_, item2, clientContainerId1);

  // Open second container's parent (i.e. the first container)
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container1), _));  // TODO: Item matcher
  container_manager_.openParentContainer(&player_ctrl_mock_, clientContainerId1);

  // Re-open the second container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container2), Ref(item2)));
  container_manager_.useContainer(&player_ctrl_mock_, item2, clientContainerId1);

  // Open first container with new clientContainerId
  const auto clientContainerId2 = 33;
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId2, Ref(*container1), Ref(item1)));
  container_manager_.useContainer(&player_ctrl_mock_, item1, clientContainerId2);

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

TEST_F(ContainerManagerTest, addRemoveItems)
{
  // Make a few dummy items
  const auto item1 = Item(1);
  const auto item2 = Item(2);
  const auto item3 = Item(3);

  // Create a container located in player inventory slot 0
  auto container_item = Item(backpack_item_id);
  const auto container_item_position = ItemPosition(GamePosition(0), container_item.getItemId(), 0);
  container_item.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                                   container_item.getItemId(),
                                                                   container_item_position));
  const auto* container = container_manager_.getContainer(container_item.getContainerId());

  // Make sure that the container is empty
  ASSERT_TRUE(container->items.empty());

  // Using non-clientContainerId is not allowed, because the player need to have the container open
  EXPECT_FALSE(container_manager_.canAddItem(&player_ctrl_mock_, container_item.getContainerId(), 0, item1));

  // Open the container
  const auto clientContainerId = 0;
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, Ref(*container), Ref(container_item)));
  container_manager_.useContainer(&player_ctrl_mock_, container_item, clientContainerId);

  // It should now be possible to add the item, but only if using clientContainerId
  // And it should be possible in both slots
  EXPECT_FALSE(container_manager_.canAddItem(&player_ctrl_mock_, container_item.getContainerId(), 0, item1));
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 0, item1));
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 1, item1));

  // Add the first item
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId, Ref(item1)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId, 0, item1);

  // There should now be one item in the container
  ASSERT_EQ(1u, container->items.size());
  EXPECT_EQ(item1.getItemId(), container->items[0].getItemId());

  // It should still be possible to add an item, in both slots
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 0, item2));
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 1, item2));

  // Add a second item
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId, Ref(item2)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId, 0, item2);

  // There should now be two items in the container, the last one added should be first in the container
  ASSERT_EQ(2u, container->items.size());
  EXPECT_EQ(item2.getItemId(), container->items[0].getItemId());
  EXPECT_EQ(item1.getItemId(), container->items[1].getItemId());

  // The container is full and it should not be possible to add another item, in neither slot
  EXPECT_FALSE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 0, item3));
  EXPECT_FALSE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 1, item3));

  // Remove the first item, which is in the second slot (slot 1)
  EXPECT_CALL(player_ctrl_mock_, onContainerRemoveItem(clientContainerId, 1));
  container_manager_.removeItem(&player_ctrl_mock_, clientContainerId, 1);

  // There should only be one item left in the container (item2)
  ASSERT_EQ(1u, container->items.size());
  EXPECT_EQ(item2.getItemId(), container->items[0].getItemId());

  // And it should now be possible to add items to it again
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 0, item3));
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 1, item3));

  // Remove the second item, at slot 0
  EXPECT_CALL(player_ctrl_mock_, onContainerRemoveItem(clientContainerId, 0));
  container_manager_.removeItem(&player_ctrl_mock_, clientContainerId, 0);

  // The container should now be empty
  ASSERT_TRUE(container->items.empty());

  // And it should still be possible to add items to it
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 0, item3));
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId, 1, item3));
}

TEST_F(ContainerManagerTest, addItemToContainerInContainer)
{
  // Make a few dummy items
  const auto item1 = Item(1);
  const auto item2 = Item(2);
  const auto item3 = Item(3);

  // Create a container located in player inventory slot 0
  auto container_item1 = Item(backpack_item_id);
  const auto container_item_position1 = ItemPosition(GamePosition(0), container_item1.getItemId(), 0);
  container_item1.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                                    container_item1.getItemId(),
                                                                    container_item_position1));
  const auto* container1 = container_manager_.getContainer(container_item1.getContainerId());

  // Create a container located in the first container
  auto container_item2 = Item(backpack_item_id);
  const auto container_item_position2 = ItemPosition(GamePosition(container_item1.getContainerId(), 0),
                                                     container_item2.getItemId(),
                                                     0);
  container_item2.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                                    container_item2.getItemId(),
                                                                    container_item_position2));
  const auto* container2 = container_manager_.getContainer(container_item2.getContainerId());

  // Have the player open the first container and then add the second container
  // to it, so that container information and item information matches
  const auto clientContainerId1 = 0;
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId1, Ref(*container1), Ref(container_item1)));
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId1, Ref(container_item2)));
  container_manager_.useContainer(&player_ctrl_mock_, container_item1, clientContainerId1);
  container_manager_.addItem(&player_ctrl_mock_, container_item1.getContainerId(), 0, container_item2);

  // We now have:
  // container1 slot0: container2
  // container1 slot1: empty
  // container2 slot0: empty
  // container2 slot1: empty
  ASSERT_EQ(1u, container1->items.size());
  EXPECT_EQ(container2->id, container1->items[0].getContainerId());
  ASSERT_EQ(0u, container2->items.size());

  // Add an item to container1 slot0 and verify that it is added to container2
  // Note that the player does not need to have container2 open, so there won't be any PlayerCtrl callbacks
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId1, 0, item1);

  // We now have:
  // container1 slot0: container2
  // container1 slot1: empty
  // container2 slot0: item1
  // container2 slot1: empty
  ASSERT_EQ(1u, container1->items.size());
  EXPECT_EQ(container2->id, container1->items[0].getContainerId());
  ASSERT_EQ(1u, container2->items.size());
  EXPECT_EQ(item1.getItemId(), container2->items[0].getItemId());

  // Add an item to container1 slot1 and verify that it is added to container1
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId1, Ref(item2)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId1, 1, item2);

  // We now have:
  // container1 slot0: item2
  // container1 slot1: container2
  // container2 slot0: item1
  // container2 slot1: empty
  ASSERT_EQ(2u, container1->items.size());
  EXPECT_EQ(item2.getItemId(), container1->items[0].getItemId());
  EXPECT_EQ(container2->id, container1->items[1].getContainerId());
  ASSERT_EQ(1u, container2->items.size());
  EXPECT_EQ(item1.getItemId(), container2->items[0].getItemId());

  // Adding an item to container1 slot0 should not be possible, but adding an item
  // to container1 slot1 should be possible
  EXPECT_FALSE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId1, 0, item3));
  EXPECT_TRUE(container_manager_.canAddItem(&player_ctrl_mock_, clientContainerId1, 1, item3));

  // Open container2
  const auto clientContainerId2 = 1;
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId2, Ref(*container2), Ref(container_item2)));
  container_manager_.useContainer(&player_ctrl_mock_, container_item2, clientContainerId2);

  // Add an item to container1 slot1, the item should go into container2 and call onContainerAddItem with clientContainerId2
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId2, Ref(item3)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId1, 1, item3);

  // We now have:
  // container1 slot0: item2
  // container1 slot1: container2
  // container2 slot0: item3
  // container2 slot1: item1
  ASSERT_EQ(2u, container1->items.size());
  EXPECT_EQ(item2.getItemId(), container1->items[0].getItemId());
  EXPECT_EQ(container2->id, container1->items[1].getContainerId());
  ASSERT_EQ(2u, container2->items.size());
  EXPECT_EQ(item3.getItemId(), container2->items[0].getItemId());
  EXPECT_EQ(item1.getItemId(), container2->items[1].getItemId());

  // Remove all items
  // Note: second item in container2 will be moved to slot 0 before it's being removed
  EXPECT_CALL(player_ctrl_mock_, onContainerRemoveItem(clientContainerId1, 0));
  EXPECT_CALL(player_ctrl_mock_, onContainerRemoveItem(clientContainerId2, 0)).Times(2);
  container_manager_.removeItem(&player_ctrl_mock_, clientContainerId1, 0);
  container_manager_.removeItem(&player_ctrl_mock_, clientContainerId2, 0);
  container_manager_.removeItem(&player_ctrl_mock_, clientContainerId2, 0);

  // We now have:
  // container1 slot0: container2
  // container1 slot1: empty
  // container2 slot0: empty
  // container2 slot1: empty
  ASSERT_EQ(1u, container1->items.size());
  EXPECT_EQ(container2->id, container1->items[0].getContainerId());
  ASSERT_EQ(0u, container2->items.size());

  // Create a third container, located in the second container
  auto container_item3 = Item(backpack_item_id);
  const auto container_item_position3 = ItemPosition(GamePosition(container_item2.getContainerId(), 0),
                                                     container_item3.getItemId(),
                                                     0);
  container_item3.setContainerId(container_manager_.createContainer(&player_ctrl_mock_,
                                                                    container_item3.getItemId(),
                                                                    container_item_position3));
  const auto* container3 = container_manager_.getContainer(container_item3.getContainerId());

  // Open container3
  const auto clientContainerId3 = 2;
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId3, Ref(*container3), Ref(container_item3)));
  container_manager_.useContainer(&player_ctrl_mock_, container_item3, clientContainerId3);

  // Add container_item3 to container2
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId2, Ref(container_item3)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId2, 0, container_item3);

  // We now have:
  // container1 slot0: container2
  // container1 slot1: empty
  // container2 slot0: container3
  // container2 slot1: empty
  // container3 slot0: empty
  // container3 slot1: empty
  ASSERT_EQ(1u, container1->items.size());
  EXPECT_EQ(container2->id, container1->items[0].getContainerId());
  ASSERT_EQ(1u, container2->items.size());
  EXPECT_EQ(container3->id, container2->items[0].getContainerId());
  ASSERT_EQ(0u, container3->items.size());

  // Add an item to container1 slot 0 and make sure that it is inserted
  // into container2, and _not_ into container3
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId2, Ref(item1)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId1, 0, item1);
}
