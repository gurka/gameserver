cmake_minimum_required(VERSION 3.0)

project(gameengine)

add_library(gameengine
  "export/container.h"
  "export/game_position.h"
  "export/player_ctrl.h"
  "export/game_engine.h"
  "export/game_engine_queue.h"
  "src/container_manager.cc"
  "src/container_manager.h"
  "src/game_engine.cc"
  "src/player.cc"
  "src/player.h"
  "src/game_engine_queue.cc"
)
