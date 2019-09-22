cmake_minimum_required(VERSION 3.13)

project(wsclient)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++17 -O0 -g --bind -s ASSERTIONS=1 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1 -s USE_SDL=2")

configure_file("${CMAKE_BINARY_DIR}/data/data.dat" "${CMAKE_BINARY_DIR}/files/data.dat" COPYONLY)
configure_file("${CMAKE_BINARY_DIR}/data/sprite.dat" "${CMAKE_BINARY_DIR}/files/sprite.dat" COPYONLY)

add_executable(wsclient
  "src/graphics.cc"
  "src/graphics.h"
  "src/network.cc"
  "src/network.h"
  "src/types.h"
  "src/wsclient.cc"
  "src/wsworld.cc"
  "src/wsworld.h"
  
  # io
  "../io/export/data_loader.h"
  "../io/export/sprite_loader.h"
  "../io/src/data_loader.cc"
  "../io/src/file_reader.h"
  "../io/src/sprite_loader.cc"

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
target_link_options(wsclient PRIVATE --preload-file "files/")
