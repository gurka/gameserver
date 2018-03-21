cmake_minimum_required(VERSION 3.0)

project(gameengine)

set(src
  "export/container.h"
  "export/player_ctrl.h"
  "export/player_manager.h"
  "export/protocol_position.h"
  "export/world_task_queue.h"
  "src/container_manager.h"
  "src/player_manager.cc"
  "src/player.cc"
  "src/player.h"
  "src/world_task_queue.cc"
)

add_library(gameengine ${src})
