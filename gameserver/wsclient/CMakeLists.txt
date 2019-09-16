cmake_minimum_required(VERSION 3.12)

project(wsclient)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -O0 -g --bind -s ASSERTIONS=1 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1")

add_executable(wsclient
  "src/graphics.cc"
  "src/graphics.h"
  "src/map.cc"
  "src/map.h"
  "src/network.cc"
  "src/network.h"
  "src/types.h"
  "src/wsclient.cc"

  # network
  "../network/export/incoming_packet.h"
  "../network/export/outgoing_packet.h"
  "../network/src/incoming_packet.cc"
  "../network/src/outgoing_packet.cc"

  # protocol
  "../protocol/export/protocol.h"
  "../protocol/export/protocol_types.h"
  "../protocol/src/protocol.cc"

  # utils
  "../utils/export/logger.h"
  "../utils/src/logger.cc"

  # world
  "../world/export/creature.h"
  "../world/export/direction.h"
  "../world/export/item.h"
  "../world/export/position.h"
  "../world/src/creature.cc"
  "../world/src/position.cc"
)
target_include_directories(wsclient PRIVATE
  "../gameengine/export"
  "../network/export"
  "../protocol/export"
  "../world/export"
  "../utils/export"
)
set_target_properties(wsclient PROPERTIES OUTPUT_NAME "index.html")