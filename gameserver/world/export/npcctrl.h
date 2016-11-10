/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Simon Sandström
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

#ifndef WORLD_NPCCTRL_H_
#define WORLD_NPCCTRL_H_

#include <string>
#include "creaturectrl.h"
#include "creature.h"
#include "position.h"
#include "item.h"

class NpcCtrl : public CreatureCtrl
{
 public:
  void onCreatureSpawn(const Creature& creature, const Position& position) override {}
  void onCreatureDespawn(const Creature& creature, const Position& position, uint8_t stackPos) override {}
  void onCreatureMove(const Creature& creature,
                      const Position& oldPosition,
                      uint8_t oldStackPos,
                      const Position& newPosition,
                      uint8_t newStackPos) override {}
  void onCreatureTurn(const Creature& creature, const Position& position, uint8_t stackPos) override {}
  void onCreatureSay(const Creature& creature, const Position& position, const std::string& message) override {}

  void onItemRemoved(const Position& position, uint8_t stackPos) override {}
  void onItemAdded(const Item& item, const Position& position) override {}

  void onTileUpdate(const Position& position) override {}
};

#endif  // WORLD_NPCCTRL_H_
