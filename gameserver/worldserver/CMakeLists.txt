cmake_minimum_required(VERSION 3.0)

project(worldserver)

set(src
  "src/protocol_71.cc"
  "src/protocol_71.h"
  "src/protocol.h"
  "src/worldserver.cc"
)

add_executable(worldserver ${src})
target_link_libraries(worldserver
  "account"
  "network"
  "gameengine"
  "world"
  "utils"
  ${LIBRARIES}
)
