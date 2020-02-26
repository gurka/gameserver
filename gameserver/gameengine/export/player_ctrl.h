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

#ifndef GAMEENGINE_EXPORT_PLAYER_CTRL_H_
#define GAMEENGINE_EXPORT_PLAYER_CTRL_H_

#include <array>
#include <string>
#include <vector>

#include "creature_ctrl.h"
#include "creature.h"
#include "item.h"

namespace common
{

class Item;

}

namespace gameengine
{

struct Container;
class Player;

class PlayerCtrl : public world::CreatureCtrl
{
 public:
  ~PlayerCtrl() override = default;

  // Called by GameEngine
  virtual common::CreatureId getPlayerId() const = 0;
  virtual void setPlayerId(common::CreatureId player_id) = 0;

  virtual void onEquipmentUpdated(const Player& player, std::uint8_t inventory_index) = 0;

  virtual void onOpenContainer(std::uint8_t new_container_id, const Container& container, const common::Item& item) = 0;
  virtual void onCloseContainer(common::ItemUniqueId container_item_unique_id, bool reset_container_id) = 0;

  virtual void onContainerAddItem(common::ItemUniqueId container_item_unique_id, const common::Item& item) = 0;
  virtual void onContainerUpdateItem(common::ItemUniqueId container_item_unique_id,
                                     std::uint8_t container_slot,
                                     const common::Item& item) = 0;
  virtual void onContainerRemoveItem(common::ItemUniqueId container_item_unique_id, std::uint8_t container_slot) = 0;

  virtual void sendTextMessage(std::uint8_t message_type, const std::string& message) = 0;

  virtual void sendCancel(const std::string& message) = 0;
  virtual void cancelMove() = 0;

  // Called by ContainerManager
  virtual const std::array<common::ItemUniqueId, 64>& getContainerIds() const = 0;
  virtual bool hasContainerOpen(common::ItemUniqueId item_unique_id) const = 0;
};

}  // namespace gameengine

#endif  // GAMEENGINE_EXPORT_PLAYER_CTRL_H_
