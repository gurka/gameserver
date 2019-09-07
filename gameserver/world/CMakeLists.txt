cmake_minimum_required(VERSION 3.12)

project(world)
if(NOT GAMESERVER_WSCLIENT)
  add_library(world
    "export/creature_ctrl.h"
    "export/creature.h"
    "export/direction.h"
    "export/item.h"
    "export/position.h"
    "export/tile.h"
    "export/world_interface.h"
    "export/world.h"
    "src/creature.cc"
    "src/position.cc"
    "src/tile.cc"
    "src/world.cc"
  )
else()
  add_library(world
    "export/creature.h"
    "export/direction.h"
    "export/item.h"
    "export/position.h"
    "export/tile.h"
    "src/creature.cc"
    "src/position.cc"
    "src/tile.cc"
  )
endif()
target_include_directories(world PUBLIC "export")
target_link_libraries(world
  utils
)
