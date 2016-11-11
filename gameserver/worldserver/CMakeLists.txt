cmake_minimum_required(VERSION 3.0)

project(worldserver)

set(src
  "src/gameengine.cc"
  "src/gameengine.h"
  "src/gameengineproxy.h"
  "src/player.cc"
  "src/player.h"
  "src/protocol_71.cc"
  "src/protocol_71.h"
  "src/protocol.h"
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

set_property(TARGET worldserver PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
