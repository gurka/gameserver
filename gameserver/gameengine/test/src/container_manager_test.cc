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
using ::testing::ReturnRef;
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

// TODO(simon): add helper functions

class ContainerManagerTest : public ::testing::Test
{
 protected:
  ContainerManagerTest()
  {
    EXPECT_CALL(player_ctrl_mock_, getPlayerId()).WillRepeatedly(Return(player_id));

    containerIds.fill(Item::INVALID_UNIQUE_ID);

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
    EXPECT_CALL(player_ctrl_mock_, getContainerIds()).WillOnce(ReturnRef(containerIds));
    container_manager_.playerDespawn(&player_ctrl_mock_);

    // TODO(simon): Verify that no containers have related players?
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

  std::array<ItemUniqueId, 64> containerIds;
};

TEST_F(ContainerManagerTest, useContainer)
{
  // Create/open a container located in player inventory slot 0
  const auto itemContainerAPos = ItemPosition(GamePosition(0), itemContainerA.getItemTypeId());
  const auto clientContainerIdA = 1u;
  // TODO: SaveArg<1> and match with getContainer
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdA, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerIdA);
  containerIds[clientContainerIdA] = itemContainerA.getItemUniqueId();

  // Validate the newly created container
  auto* containerA = container_manager_.getContainer(itemContainerA.getItemUniqueId());
  ASSERT_TRUE(containerA != nullptr);
  EXPECT_EQ(0, containerA->weight);
  EXPECT_EQ(itemContainerA, *containerA->item);
  EXPECT_EQ(Item::INVALID_UNIQUE_ID, containerA->parentItemUniqueId);
  EXPECT_EQ(itemContainerAPos, containerA->rootItemPosition);
  EXPECT_TRUE(containerA->items.empty());
  EXPECT_EQ(1u, containerA->relatedPlayers.size());
  EXPECT_EQ(1u,                 containerA->relatedPlayers.size());
  EXPECT_EQ(&player_ctrl_mock_, containerA->relatedPlayers.front());

  // Create/open a container located in the world
  const auto itemContainerBPos = ItemPosition(GamePosition(Position(1, 2, 3)), itemContainerB.getItemTypeId());
  const auto clientContainerIdB = 2u;
  // TODO: SaveArg<1> and match with getContainer
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerB.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdB, _, Ref(itemContainerB)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerB, itemContainerBPos, clientContainerIdB);
  containerIds[clientContainerIdB] = itemContainerB.getItemUniqueId();

  // Validate the newly created container
  auto* containerB = container_manager_.getContainer(itemContainerB.getItemUniqueId());
  ASSERT_TRUE(containerB != nullptr);
  EXPECT_EQ(0, containerB->weight);
  EXPECT_EQ(itemContainerB, *containerB->item);
  EXPECT_EQ(Item::INVALID_UNIQUE_ID, containerB->parentItemUniqueId);
  EXPECT_EQ(itemContainerBPos, containerB->rootItemPosition);
  EXPECT_TRUE(containerB->items.empty());
  EXPECT_EQ(1u, containerB->relatedPlayers.size());
  EXPECT_EQ(1u,                 containerB->relatedPlayers.size());
  EXPECT_EQ(&player_ctrl_mock_, containerB->relatedPlayers.front());

  // Make sure that both containers exist
  EXPECT_EQ(containerA, container_manager_.getContainer(itemContainerA.getItemUniqueId()));
  EXPECT_EQ(containerB, container_manager_.getContainer(itemContainerB.getItemUniqueId()));
}

TEST_F(ContainerManagerTest, useContainerWithSameId)
{
  // Create/open a container located in player inventory slot 0
  const auto itemContainerAPos = ItemPosition(GamePosition(0), itemContainerA.getItemTypeId());
  const auto clientContainerId = 1u;
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerId);
  containerIds[clientContainerId] = itemContainerA.getItemUniqueId();

  // Create/open a container located in the world, with same id as previous container
  const auto itemContainerBPos = ItemPosition(GamePosition(Position(1, 2, 3)), itemContainerB.getItemTypeId());
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerB.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, _, Ref(itemContainerB)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerB, itemContainerBPos, clientContainerId);
  containerIds[clientContainerId] = itemContainerB.getItemUniqueId();
}

TEST_F(ContainerManagerTest, closeContainer)
{
  // Create/open a container located in player inventory slot 0
  const auto itemContainerAPos = ItemPosition(GamePosition(0), itemContainerA.getItemTypeId());
  const auto clientContainerIdA = 1u;
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdA, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerIdA);
  containerIds[clientContainerIdA] = itemContainerA.getItemUniqueId();

  // Use it again to close the container
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(true));
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(itemContainerA.getItemUniqueId(), false));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerIdA);

  // We need to ack by calling closeContainer
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(itemContainerA.getItemUniqueId(), true));
  container_manager_.closeContainer(&player_ctrl_mock_, itemContainerA.getItemUniqueId());
  containerIds[clientContainerIdA] = Item::INVALID_UNIQUE_ID;

  // Use the item again to open the container
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdA, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerIdA);
  containerIds[clientContainerIdA] = itemContainerA.getItemUniqueId();

  // Close it without "using" the item
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(itemContainerA.getItemUniqueId(), true));
  container_manager_.closeContainer(&player_ctrl_mock_, itemContainerA.getItemUniqueId());
  containerIds[clientContainerIdA] = Item::INVALID_UNIQUE_ID;
}

TEST_F(ContainerManagerTest, innerContainer)
{
  // Create/open a container located in player inventory slot 0
  const auto itemContainerAPos = ItemPosition(GamePosition(0), itemContainerA.getItemTypeId());
  const auto clientContainerIdA = 1u;
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdA, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerAPos, clientContainerIdA);
  containerIds[clientContainerIdA] = itemContainerA.getItemUniqueId();

  // Add a regular item (slot 19, at the end of the container)
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemNotContainerA)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 19, &itemNotContainerA);

  // Add another container
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemContainerB)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 19, &itemContainerB);

  // Add a regular item
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemNotContainerB)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 19, &itemNotContainerB);

  // The container should now contain:
  // itemContainerA:
  //   0: itemNotContainerB
  //   1: itemContainerB
  //   2: itemNotContainerA
  const auto* containerA = container_manager_.getContainer(itemContainerA.getItemUniqueId());
  ASSERT_EQ(3u, containerA->items.size());
  EXPECT_EQ(itemNotContainerB, *(containerA->items[0]));
  EXPECT_EQ(itemContainerB,    *(containerA->items[1]));
  EXPECT_EQ(itemNotContainerA, *(containerA->items[2]));

  // Now add a regular item to containerA, stackPosition 1
  // This should add the item to containerB as itemContainerB is at stackPosition 1
  // containerB is not open so there should not be any onContainerAddItem call
  // Before the call there should not exist a container for itemContainerB
  // After the call a container should exist
  EXPECT_TRUE(container_manager_.getContainer(itemContainerB.getItemUniqueId()) == nullptr);
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 1, &itemNotContainerC);
  EXPECT_TRUE(container_manager_.getContainer(itemContainerB.getItemUniqueId()) != nullptr);

  // Now open the new container
  const auto clientContainerIdB = 2u;
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerB.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdB, _, Ref(itemContainerB)));
  container_manager_.useContainer(&player_ctrl_mock_,
                                  itemContainerB,
                                  ItemPosition(GamePosition(itemContainerA.getItemUniqueId(), 1),
                                               itemContainerA.getItemTypeId(),
                                               0),
                                  clientContainerIdB);
  containerIds[clientContainerIdB] = itemContainerB.getItemUniqueId();

  // Current container structure should now be:
  // itemContainerA:
  //   0: itemNotContainerB
  //   1: ContainerB
  //     0: itemNotContainerC
  //   2: itemNotContainerA
  // Verify:
  {
    const auto containerA = container_manager_.getContainer(itemContainerA.getItemUniqueId());
    const auto containerB = container_manager_.getContainer(itemContainerB.getItemUniqueId());

    EXPECT_EQ(itemContainerA, *(containerA->item));
    ASSERT_EQ(3u, containerA->items.size());
    EXPECT_EQ(itemNotContainerB, *(containerA->items[0]));
    EXPECT_EQ(itemContainerB,    *(containerA->items[1]));
    EXPECT_EQ(itemNotContainerA, *(containerA->items[2]));

    EXPECT_EQ(itemContainerB, *(containerB->item));
    ASSERT_EQ(1u, containerB->items.size());
    EXPECT_EQ(itemNotContainerC, *(containerB->items[0]));

    // Verify parentContainer and rootPosition
    EXPECT_EQ(Item::INVALID_UNIQUE_ID,          containerA->parentItemUniqueId);
    EXPECT_EQ(itemContainerA.getItemUniqueId(), containerB->parentItemUniqueId);
    EXPECT_EQ(itemContainerAPos,                containerA->rootItemPosition);
    EXPECT_EQ(itemContainerAPos,                containerB->rootItemPosition);
  }
}

TEST_F(ContainerManagerTest, playerDespawn)
{
  // TODO
}

TEST_F(ContainerManagerTest, multiplePlayers)
{
  // TODO
}

TEST_F(ContainerManagerTest, moveContainer)
{

}
