cmake_minimum_required(VERSION 3.0)

project(gameengine)

set(src
  "export/container.h"
  "export/item_position.h"
  "export/player_ctrl.h"
  "export/game_engine.h"
  "export/game_engine_queue.h"
  "src/container_manager.h"
  "src/game_engine.cc"
  "src/player.cc"
  "src/player.h"
  "src/game_engine_queue.cc"
)

add_library(gameengine ${src})
