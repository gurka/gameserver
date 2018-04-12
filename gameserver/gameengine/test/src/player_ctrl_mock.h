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

class PlayerCtrlMock : public PlayerCtrl
{
 public:
  MOCK_METHOD3(onCreatureSpawn, void(const WorldInterface& world_interface,
                                     const Creature& creature,
                                     const Position& position));

  MOCK_METHOD4(onCreatureDespawn, void(const WorldInterface& world_interface,
                                       const Creature& creature,
                                       const Position& position,
                                       uint8_t stackPos));

  MOCK_METHOD6(onCreatureMove, void(const WorldInterface& world_interface,
                                    const Creature& creature,
                                    const Position& oldPosition,
                                    uint8_t oldStackPos,
                                    const Position& newPosition,
                                    uint8_t newStackPos));

  MOCK_METHOD4(onCreatureTurn, void(const WorldInterface& world_interface,
                                    const Creature& creature,
                                    const Position& position,
                                    uint8_t stackPos));

  MOCK_METHOD4(onCreatureSay, void(const WorldInterface& world_interface,
                                   const Creature& creature,
                                   const Position& position,
                                   const std::string& message));

  MOCK_METHOD3(onItemRemoved, void(const WorldInterface& world_interface,
                                   const Position& position,
                                   uint8_t stackPos));

  MOCK_METHOD3(onItemAdded, void (const WorldInterface& world_interface,
                                  const Item& item,
                                  const Position& position));

  MOCK_METHOD2(onTileUpdate, void(const WorldInterface& world_interface,
                                  const Position& position));

  MOCK_CONST_METHOD0(getPlayerId, CreatureId());
  MOCK_METHOD1(setPlayerId, void(CreatureId playerId));

  MOCK_METHOD2(onEquipmentUpdated, void(const Player& player, int inventoryIndex));

  MOCK_METHOD3(onOpenContainer, void(uint8_t clientContainerId, const Container& container, const Item& item));
  MOCK_METHOD1(onCloseContainer, void(uint8_t clientContainerId));

  MOCK_METHOD2(onContainerAddItem, void(uint8_t clientContainerId, const Item& item));
  MOCK_METHOD3(onContainerUpdateItem, void(uint8_t clientContainerId, int containerSlot, const Item& item));
  MOCK_METHOD2(onContainerRemoveItem, void(uint8_t clientContainerId, int containerSlot));

  MOCK_METHOD2(sendTextMessage, void(uint8_t message_type, const std::string& message));

  MOCK_METHOD1(sendCancel, void(const std::string& message));
  MOCK_METHOD0(cancelMove, void());
};

#endif  // GAMEENGINE_TEST_PLAYERCTRL_MOCK_H_
