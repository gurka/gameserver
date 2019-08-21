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
using ::testing::SaveArg;

struct ItemStub : public Item
{
  ItemStub()
      : itemUniqueId_(0u),
        itemTypeId_(0),
        itemType_(),
        count_(0)
  {
  }

  ItemStub(ItemUniqueId itemUniqueId,
           ItemTypeId itemTypeId,
           ItemType itemType,
           int count)
      : itemUniqueId_(itemUniqueId),
        itemTypeId_(itemTypeId),
        itemType_(itemType),
        count_(count)
  {
  }

  ItemUniqueId getItemUniqueId() const { return itemUniqueId_; }
  ItemTypeId getItemTypeId() const { return itemTypeId_; }

  const ItemType& getItemType() const { return itemType_; }

  int getCount() const { return count_; }
  void setCount(int count) { count_ = count; }

  ItemUniqueId itemUniqueId_;
  ItemTypeId itemTypeId_;
  ItemType itemType_;
  int count_;
};

class ContainerManagerTest : public ::testing::Test
{
 protected:
  ContainerManagerTest()
  {
    EXPECT_CALL(player_ctrl_mock_, getPlayerId()).WillRepeatedly(Return(player_id));

    // Inform ContainerManager about this player
    container_manager_.playerSpawn(&player_ctrl_mock_);

    itemTypeContainer.id = 123;
    itemTypeContainer.isContainer = true;

    itemTypeNotContainer.id = 456;
    itemTypeNotContainer.isContainer = false;

    itemContainerA = ItemStub { 100u, itemTypeContainer.id, itemTypeContainer, 1 };
    itemContainerB = ItemStub { 101u, itemTypeContainer.id, itemTypeContainer, 1 };
    itemContainerC = ItemStub { 102u, itemTypeContainer.id, itemTypeContainer, 1 };

    itemNotContainerA = ItemStub { 200u, itemTypeNotContainer.id, itemTypeNotContainer, 1 };
    itemNotContainerB = ItemStub { 201u, itemTypeNotContainer.id, itemTypeNotContainer, 2 };
    itemNotContainerC = ItemStub { 202u, itemTypeNotContainer.id, itemTypeNotContainer, 3 };
  }

  ~ContainerManagerTest()
  {
    container_manager_.playerDespawn(&player_ctrl_mock_);
  }

  PlayerCtrlMock player_ctrl_mock_;
  ContainerManager container_manager_;
  const CreatureId player_id = 123;

  ItemType itemTypeContainer;
  ItemType itemTypeNotContainer;

  ItemStub itemContainerA;
  ItemStub itemContainerB;
  ItemStub itemContainerC;
  ItemStub itemNotContainerA;
  ItemStub itemNotContainerB;
  ItemStub itemNotContainerC;
};

TEST_F(ContainerManagerTest, useContainer)
{
  const auto itemContainerAPos = ItemPosition(GamePosition(0), itemContainerA.getItemTypeId(), 0);
  const auto clientContainerId = 1u;

  // Using itemA will create and open its container
  Container containerA;
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, _, Ref(itemContainerA))).WillOnce(SaveArg<1>(&containerA));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerId);

  // Validate the container
  EXPECT_EQ(&itemContainerA,    containerA.item);
  EXPECT_TRUE(containerA.items.empty());
  EXPECT_EQ(-1,                 containerA.parentContainerId);
  EXPECT_EQ(itemContainerAPos,  containerA.rootItemPosition);
  EXPECT_EQ(1u,                 containerA.relatedPlayers.size());
  EXPECT_EQ(&player_ctrl_mock_, containerA.relatedPlayers.front().playerCtrl);
  EXPECT_EQ(clientContainerId,  containerA.relatedPlayers.front().clientContainerId);

  // Retrieve the container using containerId and validate it against the original
  const auto* containerA2 = container_manager_.getContainer(containerA.id);
  ASSERT_TRUE(containerA2 != nullptr);
  EXPECT_EQ(containerA.id,                    containerA2->id);
  EXPECT_EQ(containerA.item,                  containerA2->item);
  EXPECT_EQ(containerA.items,                 containerA2->items);
  EXPECT_EQ(containerA.parentContainerId,     containerA2->parentContainerId);
  EXPECT_EQ(containerA.relatedPlayers,        containerA2->relatedPlayers);
  EXPECT_EQ(containerA.rootItemPosition,      containerA2->rootItemPosition);
  EXPECT_EQ(containerA.weight,                containerA2->weight);

  // Retrieve the container using clientContainerId and playerCtrl, and validate it against the original
  const auto* containerA3 = container_manager_.getContainer(&player_ctrl_mock_, clientContainerId);
  ASSERT_TRUE(containerA3 != nullptr);
  EXPECT_EQ(containerA.id,                    containerA3->id);
  EXPECT_EQ(containerA.item,                  containerA3->item);
  EXPECT_EQ(containerA.items,                 containerA3->items);
  EXPECT_EQ(containerA.parentContainerId,     containerA3->parentContainerId);
  EXPECT_EQ(containerA.relatedPlayers,        containerA3->relatedPlayers);
  EXPECT_EQ(containerA.rootItemPosition,      containerA3->rootItemPosition);
  EXPECT_EQ(containerA.weight,                containerA3->weight);
}

TEST_F(ContainerManagerTest, closeContainer)
{
  const auto itemContainerAPos = ItemPosition(GamePosition(0), itemContainerA.getItemTypeId(), 0);
  const auto clientContainerId = 1u;

  // Use the item to create and open the container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerId);

  // Use it again to close the container
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(clientContainerId));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerId);

  // We need to ack by calling closeContainer
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(clientContainerId));
  container_manager_.closeContainer(&player_ctrl_mock_, clientContainerId);

  // Use the item again to open the container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerId);

  // Close it without "using" the item
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(clientContainerId));
  container_manager_.closeContainer(&player_ctrl_mock_, clientContainerId);
}

TEST_F(ContainerManagerTest, innerContainer)
{
  const auto itemPosition = ItemPosition(GamePosition(0), itemContainerA.getItemTypeId(), 0);
  const auto clientContainerId = 1u;

  // TODO: what stackPosition does the use when dragging an item into an empty spot in a container?
  const auto stackPosition = -1;

  // Use the item to create and open the container
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemPosition, clientContainerId);

  // Add a regular item
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId, Ref(itemNotContainerA)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId, stackPosition, &itemNotContainerA);

  // Add another container
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId, Ref(itemContainerB)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId, stackPosition, &itemContainerB);

  // Add a regular item
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(clientContainerId, Ref(itemNotContainerB)));
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId, stackPosition, &itemNotContainerB);

  // The container should now contain:
  // itemContainerA: [itemNotContainerB, itemContainerB, itemNotContainerA]
  const auto* containerA = container_manager_.getContainer(&player_ctrl_mock_, clientContainerId);
  EXPECT_EQ(3u, containerA->items.size());
  // TODO: comparison functions on Item?
  EXPECT_EQ(itemNotContainerB.getItemUniqueId(), containerA->items[0]->getItemUniqueId());
  EXPECT_EQ(itemContainerB.getItemUniqueId(),    containerA->items[1]->getItemUniqueId());
  EXPECT_EQ(itemNotContainerA.getItemUniqueId(), containerA->items[2]->getItemUniqueId());

  // Now add a regular item to containerA, stackPosition 1
  // This should add the item to containerB as itemContainerB is at stackPosition 1
  // containerB is not open so there should not be any onContainerAddItem call
  container_manager_.addItem(&player_ctrl_mock_, clientContainerId, 1, &itemNotContainerC);

  // TODO: [container_manager.cc:343] ERROR: addItem: create new Container for inner container NOT YET IMPLEMENTED
}
