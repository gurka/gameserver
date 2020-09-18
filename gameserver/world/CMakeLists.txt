cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_library(world
  "export/creature_ctrl.h"
  "export/tile.h"
  "export/world.h"
  "src/tile.cc"
  "src/world.cc"
)

target_link_libraries(world PRIVATE
  common
  utils
)

target_include_directories(world PUBLIC "export")
