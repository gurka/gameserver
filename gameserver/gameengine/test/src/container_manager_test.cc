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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "player.h"
#include "container_manager.h"
#include "player_ctrl_mock.h"

namespace gameengine
{

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Ref;
using ::testing::_;
using ::testing::SaveArg;
using ::testing::WithArg;
using ::testing::Invoke;

using namespace common;

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

  std::uint8_t getCount() const { return count_; }
  void setCount(std::uint8_t count) { count_ = count; }

  ItemUniqueId itemUniqueId_;
  ItemTypeId itemTypeId_;
  ItemType itemType_;
  std::uint8_t count_;
};

class ContainerManagerTest : public ::testing::Test
{
 protected:
  ContainerManagerTest()
  {
    EXPECT_CALL(player_ctrl_mock_, getPlayerId()).WillRepeatedly(Return(901564));
    EXPECT_CALL(player_ctrl_mock_, getContainerIds()).WillRepeatedly(ReturnRef(containerIds));

    containerIds.fill(Item::INVALID_UNIQUE_ID);

    ItemType itemTypeContainer;
    itemTypeContainer.id = 123;
    itemTypeContainer.is_container = true;

    ItemType itemTypeNotContainer;
    itemTypeNotContainer.id = 456;
    itemTypeNotContainer.is_container = false;

    itemContainerA = ItemStub { 100u, itemTypeContainer.id, itemTypeContainer, 1 };
    itemContainerB = ItemStub { 101u, itemTypeContainer.id, itemTypeContainer, 1 };
    itemContainerC = ItemStub { 102u, itemTypeContainer.id, itemTypeContainer, 1 };

    itemContainerPosA = GamePosition(0);
    itemContainerPosB = GamePosition(Position(1, 2, 3));

    clientContainerIdA = 1;
    clientContainerIdB = 2;

    itemNotContainerA = ItemStub { 200u, itemTypeNotContainer.id, itemTypeNotContainer, 1 };
    itemNotContainerB = ItemStub { 201u, itemTypeNotContainer.id, itemTypeNotContainer, 2 };
    itemNotContainerC = ItemStub { 202u, itemTypeNotContainer.id, itemTypeNotContainer, 3 };
  }

  ~ContainerManagerTest()
  {
    container_manager_.playerDespawn(&player_ctrl_mock_);

    EXPECT_TRUE(container_manager_.noRelatedPlayers());
  }

  Container* createAndOpenContainer(const ItemStub& itemContainer,
                                    const GamePosition& gamePosition,
                                    int clientContainerId)
  {
    const Container* savedContainer = nullptr;
    const auto saveContainer = [&savedContainer](const Container& container)
    {
      savedContainer = &container;
    };

    EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainer.getItemUniqueId())).WillOnce(Return(false));
    EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerId, _, Ref(itemContainer)))
      .WillOnce(WithArg<1>(Invoke(saveContainer)));
    container_manager_.useContainer(&player_ctrl_mock_, itemContainer, gamePosition, clientContainerId);
    containerIds[clientContainerId] = itemContainer.getItemUniqueId();
    if (!savedContainer)
    {
      return nullptr;
    }
    EXPECT_EQ(savedContainer, container_manager_.getContainer(itemContainer.getItemUniqueId()));
    return container_manager_.getContainer(itemContainer.getItemUniqueId());
  }

  PlayerCtrlMock player_ctrl_mock_;
  ContainerManager container_manager_;

  ItemStub itemContainerA;
  ItemStub itemContainerB;
  ItemStub itemContainerC;

  GamePosition itemContainerPosA;
  GamePosition itemContainerPosB;

  int clientContainerIdA;
  int clientContainerIdB;

  ItemStub itemNotContainerA;
  ItemStub itemNotContainerB;
  ItemStub itemNotContainerC;

  std::array<ItemUniqueId, 64> containerIds;
};

TEST_F(ContainerManagerTest, useContainer)
{
  // Create, use/open and validate a new container
  auto* containerA = createAndOpenContainer(itemContainerA, itemContainerPosA, clientContainerIdA);
  ASSERT_TRUE(containerA != nullptr);
  EXPECT_EQ(0,                       containerA->weight);
  EXPECT_EQ(itemContainerA,         *containerA->item);
  EXPECT_EQ(Item::INVALID_UNIQUE_ID, containerA->parent_item_unique_id);
  EXPECT_EQ(itemContainerPosA,       containerA->root_game_position);
  EXPECT_TRUE(containerA->items.empty());
  EXPECT_EQ(1u,                      containerA->related_players.size());
  EXPECT_EQ(&player_ctrl_mock_,      containerA->related_players.front());

  // Create, use/open and validate a new container
  auto* containerB = createAndOpenContainer(itemContainerB, itemContainerPosB, clientContainerIdB);
  ASSERT_TRUE(containerB != nullptr);
  EXPECT_EQ(0,                       containerB->weight);
  EXPECT_EQ(itemContainerB,         *containerB->item);
  EXPECT_EQ(Item::INVALID_UNIQUE_ID, containerB->parent_item_unique_id);
  EXPECT_EQ(itemContainerPosB,       containerB->root_game_position);
  EXPECT_TRUE(containerB->items.empty());
  EXPECT_EQ(1u,                      containerB->related_players.size());
  EXPECT_EQ(&player_ctrl_mock_,      containerB->related_players.front());

  std::cout << containerA->toString() << '\n';
  std::cout << containerB->toString() << '\n';
}

TEST_F(ContainerManagerTest, useContainerWithSameId)
{
  // Create/open a container located in player inventory slot 0
  const auto* containerA = createAndOpenContainer(itemContainerA, itemContainerPosA, clientContainerIdA);

  // Create/open a container located in the world, with same id as previous container
  const auto* containerB = createAndOpenContainer(itemContainerB, itemContainerPosB, clientContainerIdA);

  std::cout << containerA->toString() << '\n';
  std::cout << containerB->toString() << '\n';
}

TEST_F(ContainerManagerTest, closeContainer)
{
  // Create/open a container located in player inventory slot 0
  const auto* containerA = createAndOpenContainer(itemContainerA, itemContainerPosA, clientContainerIdA);

  // Use it again to close the container
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(true));
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(itemContainerA.getItemUniqueId(), false));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerPosA, clientContainerIdA);

  // We need to ack by calling closeContainer
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(itemContainerA.getItemUniqueId(), true));
  container_manager_.closeContainer(&player_ctrl_mock_, itemContainerA.getItemUniqueId());
  containerIds[clientContainerIdA] = Item::INVALID_UNIQUE_ID;

  // Use the item again to open the container
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerA.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdA, _, Ref(itemContainerA)));
  container_manager_.useContainer(&player_ctrl_mock_, itemContainerA, itemContainerPosA, clientContainerIdA);
  containerIds[clientContainerIdA] = itemContainerA.getItemUniqueId();

  // Close it without "using" the item
  EXPECT_CALL(player_ctrl_mock_, onCloseContainer(itemContainerA.getItemUniqueId(), true));
  container_manager_.closeContainer(&player_ctrl_mock_, itemContainerA.getItemUniqueId());
  containerIds[clientContainerIdA] = Item::INVALID_UNIQUE_ID;

  std::cout << containerA->toString() << '\n';
}

TEST_F(ContainerManagerTest, innerContainer)
{
  // Create/open a container located in player inventory slot 0
  const auto* containerA = createAndOpenContainer(itemContainerA, itemContainerPosA, clientContainerIdA);

  // Add a regular item (slot 19, at the end of the container)
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemNotContainerA)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 19, itemNotContainerA);

  // Add another container
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemContainerB)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 19, itemContainerB);

  // Add a regular item
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemNotContainerB)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 19, itemNotContainerB);

  // The container should now contain:
  // itemContainerA:
  //   0: itemNotContainerB
  //   1: itemContainerB
  //   2: itemNotContainerA
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
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 1, itemNotContainerC);
  EXPECT_TRUE(container_manager_.getContainer(itemContainerB.getItemUniqueId()) != nullptr);

  // Now open the new container
  EXPECT_CALL(player_ctrl_mock_, hasContainerOpen(itemContainerB.getItemUniqueId())).WillOnce(Return(false));
  EXPECT_CALL(player_ctrl_mock_, onOpenContainer(clientContainerIdB, _, Ref(itemContainerB)));
  container_manager_.useContainer(&player_ctrl_mock_,
                                  itemContainerB,
                                  GamePosition(itemContainerA.getItemUniqueId(), 1),
                                  clientContainerIdB);
  containerIds[clientContainerIdB] = itemContainerB.getItemUniqueId();

  // Current container structure should now be:
  // itemContainerA:
  //   0: itemNotContainerB
  //   1: ContainerB
  //     0: itemNotContainerC
  //   2: itemNotContainerA
  // Verify:
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
  EXPECT_EQ(Item::INVALID_UNIQUE_ID,          containerA->parent_item_unique_id);
  EXPECT_EQ(itemContainerA.getItemUniqueId(), containerB->parent_item_unique_id);
  EXPECT_EQ(itemContainerPosA,                containerA->root_game_position);
  EXPECT_EQ(itemContainerPosA,                containerB->root_game_position);

  std::cout << containerA->toString() << '\n';
  std::cout << containerB->toString() << '\n';
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
  // Create/open a container located in player inventory slot 0
  const auto* containerA = createAndOpenContainer(itemContainerA, itemContainerPosA, clientContainerIdA);

  // Add a regular item
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemNotContainerA)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 0, itemNotContainerA);

  // Add another container
  EXPECT_CALL(player_ctrl_mock_, onContainerAddItem(itemContainerA.getItemUniqueId(), Ref(itemContainerB)));
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 0, itemContainerB);

  // Add a regular item (will be placed in the inner container)
  // No onContainerAddItem as the player doesn't have the inner container open
  container_manager_.addItem(itemContainerA.getItemUniqueId(), 0, itemNotContainerB);

  // Just get container B without opening it
  const auto containerB = container_manager_.getContainer(itemContainerB.getItemUniqueId());

  const auto worldPositionA = GamePosition(Position(0, 0, 0));
  const auto worldPositionB = GamePosition(Position(1, 1, 1));
  const auto inventoryPositionA = GamePosition(5);

  // Both container A and B's root_game_position should be itemContainerPosA
  EXPECT_EQ(itemContainerPosA, containerA->root_game_position);
  EXPECT_EQ(itemContainerPosA, containerB->root_game_position);

  // Move container A to a world position A
  container_manager_.updateRootPosition(itemContainerA.getItemUniqueId(), worldPositionA);

  // Both container A and B's root_game_position should be changed to the world position A
  EXPECT_EQ(worldPositionA, containerA->root_game_position);
  EXPECT_EQ(worldPositionA, containerB->root_game_position);

  // Move container B to world position B
  EXPECT_CALL(player_ctrl_mock_, onContainerRemoveItem(itemContainerA.getItemUniqueId(), 0));
  container_manager_.removeItem(itemContainerA.getItemUniqueId(), 0);
  container_manager_.updateRootPosition(itemContainerB.getItemUniqueId(), worldPositionB);

  // Container B's root_game_position should be changed to the world position B
  EXPECT_EQ(worldPositionB, containerB->root_game_position);

  // Move container A to inside container B
  container_manager_.addItem(itemContainerB.getItemUniqueId(), 0, itemContainerA);

  // Both container A and B's root_game_position should be changed to world position B
  EXPECT_EQ(worldPositionB, containerA->root_game_position);
  EXPECT_EQ(worldPositionB, containerB->root_game_position);

  // Move container B to inventory position
  container_manager_.updateRootPosition(itemContainerB.getItemUniqueId(), inventoryPositionA);

  // Both container A and B's root_game_position should be changed to inventory position
  EXPECT_EQ(inventoryPositionA, containerA->root_game_position);
  EXPECT_EQ(inventoryPositionA, containerB->root_game_position);

  std::cout << containerA->toString() << '\n';
  std::cout << containerB->toString() << '\n';
}

}  // namespace gameengine
