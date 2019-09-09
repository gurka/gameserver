cmake_minimum_required(VERSION 3.12)

project(gameengine)
if(NOT GAMESERVER_WSCLIENT)
  add_library(gameengine
    "export/container.h"
    "export/game_engine_queue.h"
    "export/game_engine.h"
    "export/game_position.h"
    "export/player_ctrl.h"
    "export/player.h"
    "src/container_manager.cc"
    "src/container_manager.h"
    "src/container.cc"
    "src/game_engine_queue.cc"
    "src/game_engine.cc"
    "src/item_manager.cc"
    "src/item_manager.h"
    "src/player.cc"
    "src/world_factory.cc"
    "src/world_factory.h"
  )
  target_link_libraries(gameengine
    utils
    world
    rapidxml
  )
else()
  add_library(gameengine
    "export/container.h"
    "export/game_position.h"
    "export/player.h"
    "src/container.cc"
    "src/player.cc"
  )
  target_link_libraries(gameengine
    utils
    world
  )
endif()
target_include_directories(gameengine PUBLIC "export")
