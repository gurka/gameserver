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

#ifndef GAMEENGINE_CONTAINERMANAGER_H_
#define GAMEENGINE_CONTAINERMANAGER_H_

#include <array>
#include <unordered_map>

#include "container.h"
#include "game_position.h"

class PlayerCtrl;
class Item;

class ContainerManager
{
 public:
  ContainerManager()
    : nextContainerId_(64),
      containers_(),
      clientContainerIds_()
  {
  }

  void playerSpawn(const PlayerCtrl* playerCtrl);
  void playerDespawn(const PlayerCtrl* playerCtrl);

  const Container* getContainer(int containerId) const;
  Container* getContainer(int containerId);
  const Container* getContainer(const PlayerCtrl* playerCtrl, int containerId) const;
  Container* getContainer(const PlayerCtrl* playerCtrl, int containerId);
  Item* getItem(const PlayerCtrl* playerCtrl, int containerId, int containerSlot);

  int createContainer(PlayerCtrl* playerCtrl, ItemId itemId, const ItemPosition& itemPosition);

  void useContainer(PlayerCtrl* playerCtrl, Item* item, const ItemPosition& itemPosition, int newClientContainerId);
  void closeContainer(PlayerCtrl* playerCtrl, int containerId);
  void openParentContainer(PlayerCtrl* playerCtrl, int containerId);

  bool canAddItem(const PlayerCtrl* playerCtrl, int containerId, int containerSlot, const Item& item) const;
  void removeItem(const PlayerCtrl* playerCtrl, int containerId, int containerSlot);
  void addItem(const PlayerCtrl* playerCtrl, int containerId, int containerSlot, const Item& item);

 private:
  void openContainer(PlayerCtrl* playerCtrl, Container* container, int clientContainerId, const Item& item);
  void closeContainer(PlayerCtrl* playerCtrl, Container* container, int clientContainerId);

  bool isClientContainerId(int containerId) const;
  void setClientContainerId(CreatureId playerId, int clientContainerId, int containerId);
  int getClientContainerId(CreatureId playerId, int containerId) const;
  int getContainerId(CreatureId playerId, int clientContainerId) const;

  void addRelatedPlayer(Container* container, PlayerCtrl* playerCtrl, int clientContainerId);
  void removeRelatedPlayer(Container* container, const PlayerCtrl* playerCtrl, int clientContainerId);

  int nextContainerId_;
  std::unordered_map<int, Container> containers_;
  std::unordered_map<CreatureId, std::array<int, 64>> clientContainerIds_;
};

#endif  // GAMEENGINE_CONTAINERMANAGER_H_
