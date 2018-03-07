cmake_minimum_required(VERSION 3.0)

project(worldserver)

set(src
  "src/player_ctrl.h"
  "src/player_manager.cc"
  "src/player_manager.h"
  "src/player.cc"
  "src/player.h"
  "src/protocol_71.cc"
  "src/protocol_71.h"
  "src/protocol.h"
  "src/world_task_queue.cc"
  "src/world_task_queue.h"
  "src/worldserver.cc"
)

add_executable(worldserver ${src})
target_link_libraries(worldserver
  "account"
  "network"
  "world"
  "utils"
  ${LIBRARIES}
)
