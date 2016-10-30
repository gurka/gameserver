cmake_minimum_required(VERSION 3.0)

project(worldserver)

set(src
  "src/gameengine.cc"
  "src/gameengine.h"
  "src/gameengineproxy.h"
  "src/player.cc"
  "src/playerctrl.cc"
  "src/playerctrl.h"
  "src/player.h"
  "src/taskqueue.h"
  "src/worldserver.cc"
)

add_executable(worldserver ${src})
target_link_libraries(worldserver
  "account"
  "network"
  "utils"
  "world"
  ${LIBRARIES}
)
