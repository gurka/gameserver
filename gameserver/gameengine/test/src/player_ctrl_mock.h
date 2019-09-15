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

#ifndef GAMEENGINE_TEST_PLAYERCTRL_MOCK_H_
#define GAMEENGINE_TEST_PLAYERCTRL_MOCK_H_

#include "gmock/gmock.h"

#include "player_ctrl.h"

namespace gameengine
{

using namespace world;

class PlayerCtrlMock : public PlayerCtrl
{
 public:
  MOCK_METHOD2(onCreatureSpawn, void(const Creature& creature,
                                     const Position& position));

  MOCK_METHOD3(onCreatureDespawn, void(const Creature& creature,
                                       const Position& position,
                                       std::uint8_t stackPos));

  MOCK_METHOD4(onCreatureMove, void(const Creature& creature,
                                    const Position& oldPosition,
                                    std::uint8_t oldStackPos,
                                    const Position& newPosition));

  MOCK_METHOD3(onCreatureTurn, void(const Creature& creature,
                                    const Position& position,
                                    std::uint8_t stackPos));

  MOCK_METHOD3(onCreatureSay, void(const Creature& creature,
                                   const Position& position,
                                   const std::string& message));

  MOCK_METHOD2(onItemRemoved, void(const Position& position,
                                   std::uint8_t stackPos));

  MOCK_METHOD2(onItemAdded, void (const Item& item,
                                  const Position& position));

  MOCK_METHOD1(onTileUpdate, void(const Position& position));

  MOCK_CONST_METHOD0(getPlayerId, CreatureId());
  MOCK_METHOD1(setPlayerId, void(CreatureId playerId));

  MOCK_METHOD2(onEquipmentUpdated, void(const Player& player, std::uint8_t inventoryIndex));

  MOCK_METHOD3(onOpenContainer, void(std::uint8_t newContainerId, const Container& container, const Item& item));
  MOCK_METHOD2(onCloseContainer, void(ItemUniqueId itemUniqueId, bool resetContainerId));

  MOCK_METHOD2(onContainerAddItem, void(ItemUniqueId itemUniqueId, const Item& item));
  MOCK_METHOD3(onContainerUpdateItem, void(ItemUniqueId itemUniqueId, std::uint8_t containerSlot, const Item& item));
  MOCK_METHOD2(onContainerRemoveItem, void(ItemUniqueId itemUniqueId, std::uint8_t containerSlot));

  MOCK_METHOD2(sendTextMessage, void(std::uint8_t message_type, const std::string& message));

  MOCK_METHOD1(sendCancel, void(const std::string& message));
  MOCK_METHOD0(cancelMove, void());

  MOCK_CONST_METHOD0(getContainerIds, const std::array<ItemUniqueId, 64>&());
  MOCK_CONST_METHOD1(hasContainerOpen, bool(ItemUniqueId itemUniqueId));
};

}  // namespace gameengine

#endif  // GAMEENGINE_TEST_PLAYERCTRL_MOCK_H_
