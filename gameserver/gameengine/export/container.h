#ifndef GAMEENGINE_CONTAINER_H_
#define GAMEENGINE_CONTAINER_H_

#include <vector>

#include "item.h"
#include "position.h"
#include "creature.h"

struct Container
{
  Container()
    : id(INVALID_ID),
      parentContainerId(INVALID_ID),
      rootContainerId(INVALID_ID),
      weight(0),
      containerItemId(Item::INVALID_ID),
      position(),
      items(),
      relatedPlayers()
  {
  }

  static constexpr int INVALID_ID = 0;
  static constexpr int PARENT_IS_TILE = 1;
  static constexpr int PARENT_IS_PLAYER = 2;
  static constexpr int VALID_ID_START = 3;

  // This Container's id
  int id;

  // This Container's parent's id or PARENT_IS_TILE / PARENT_IS_PLAYER if no parent
  int parentContainerId;

  // The root Container's id, PARENT_IS_TILE or PARENT_IS_PLAYER
  int rootContainerId;

  // The total weight of this Container and all Items in it (including other Containers)
  int weight;

  // The ItemId of this Container (e.g. ItemId of a backpack)
  ItemId containerItemId;

  // Position of the Item that this Container belongs to, only applicable if parentContainerId is PARENT_IS_TILE
  Position position;

  // Collection of Items in the Container
  std::vector<Item> items;

  // List of Players that have this Container open
  std::vector<CreatureId> relatedPlayers;
};

#endif // GAMEENGINE_CONTAINER_H_
