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

#ifndef GAMEENGINE_SRC_CONTAINER_MANAGER_H_
#define GAMEENGINE_SRC_CONTAINER_MANAGER_H_

#include <array>
#include <unordered_map>

#include "container.h"
#include "item.h"
#include "creature.h"
#include "game_position.h"

namespace common
{

class Item;

}

namespace gameengine
{

class PlayerCtrl;

class ContainerManager
{
 public:
  void playerDespawn(const PlayerCtrl* player_ctrl);

  const Container* getContainer(common::ItemUniqueId item_unique_id) const;
  Container* getContainer(common::ItemUniqueId item_unique_id);

  const common::Item* getItem(common::ItemUniqueId item_unique_id, int container_slot) const;

  void useContainer(PlayerCtrl* player_ctrl,
                    const common::Item& item,
                    const common::GamePosition& game_position,
                    int new_container_id);
  void closeContainer(PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id);
  void openParentContainer(PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id, int new_container_id);

  bool canAddItem(common::ItemUniqueId item_unique_id, int container_slot, const common::Item& item);
  void removeItem(common::ItemUniqueId item_unique_id, int container_slot);
  void addItem(common::ItemUniqueId item_unique_id, int container_slot, const common::Item& item);

  void updateRootPosition(common::ItemUniqueId item_unique_id, const common::GamePosition& game_position);

 private:
  Container* getInnerContainer(Container* container, int container_slot);

  void createContainer(const common::Item* item, const common::GamePosition& game_position);
  void openContainer(PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id, int new_container_id);

  void addRelatedPlayer(PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id);
  void removeRelatedPlayer(const PlayerCtrl* player_ctrl, common::ItemUniqueId item_unique_id);

  // Maps common::ItemUniqueId to Container
  std::unordered_map<common::ItemUniqueId, Container> m_containers;

#ifdef UNITTEST
 public:
  bool noRelatedPlayers()
  {
    for (const auto& pair : m_containers)
    {
      if (!pair.second.related_players.empty())
      {
        return false;
      }
    }
    return true;
  }
#endif
};

}  // namespace gameengine

#endif  // GAMEENGINE_SRC_CONTAINER_MANAGER_H_
