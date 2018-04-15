cmake_minimum_required(VERSION 3.0)

project(world)

add_library(world
  "export/creature_ctrl.h"
  "export/creature.h"
  "export/direction.h"
  "export/item.h"
  "export/position.h"
  "export/tile.h"
  "export/world_factory.h"
  "export/world_interface.h"
  "export/world.h"
  "src/constants.h"
  "src/creature.cc"
  "src/item.cc"
  "src/position.cc"
  "src/tile.cc"
  "src/world_factory.cc"
  "src/world.cc"
)
