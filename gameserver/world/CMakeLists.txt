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
    "export/world_factory.h"
    "export/world_interface.h"
    "export/world.h"
    "src/creature.cc"
    "src/position.cc"
    "src/item_manager.cc"
    "src/item_manager.h"
    "src/tile.cc"
    "src/world_factory.cc"
    "src/world.cc"
  )
  target_link_libraries(world
    utils
    rapidxml
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
  target_link_libraries(world
    utils
  )
endif()
target_include_directories(world PUBLIC "export")
