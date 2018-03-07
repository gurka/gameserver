cmake_minimum_required(VERSION 3.0)

project(worldserver)

set(src
  "src/gameengine.cc"
  "src/gameengine.h"
  "src/player.cc"
  "src/playerctrl.h"
  "src/player.h"
  "src/protocol_71.cc"
  "src/protocol_71.h"
  "src/protocol.h"
  "src/worldserver.cc"
  "src/worldtaskqueue.cc"
  "src/worldtaskqueue.h"
)

add_executable(worldserver ${src})
target_link_libraries(worldserver
  "account"
  "network"
  "world"
  "utils"
  ${LIBRARIES}
)
