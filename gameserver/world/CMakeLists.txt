cmake_minimum_required(VERSION 3.12)

project(world)
add_library(world
  "export/creature_ctrl.h"
  "export/creature.h"
  "export/direction.h"
  "export/item.h"
  "export/position.h"
  "export/thing.h"
  "export/tile.h"
  "export/world.h"
  "src/creature.cc"
  "src/position.cc"
  "src/tile.cc"
  "src/world.cc"
)
target_link_libraries(world
  utils
)
target_include_directories(world PUBLIC "export")
