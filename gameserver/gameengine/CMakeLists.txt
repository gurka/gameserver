cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_library(gameengine
  "export/container.h"
  "export/game_engine.h"
  "export/game_engine_queue.h"
  "export/player.h"
  "export/player_ctrl.h"
  "src/container.cc"
  "src/container_manager.cc"
  "src/container_manager.h"
  "src/game_engine.cc"
  "src/game_engine_queue.cc"
  "src/item_manager.cc"
  "src/item_manager.h"
  "src/player.cc"
  "src/world_factory.cc"
  "src/world_factory.h"
  "src/world_loader.cc"
  "src/world_loader.h"
)

target_link_libraries(gameengine PRIVATE
  common
  utils
  world
  asio
  rapidxml
)

target_include_directories(gameengine PUBLIC "export")
