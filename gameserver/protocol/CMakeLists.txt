cmake_minimum_required(VERSION 3.12)

project(protocol)

add_library(protocol
  "export/protocol.h"
  "export/protocol_types.h"
  "src/protocol.cc"
)

target_link_libraries(protocol
  PRIVATE gameengine
  PRIVATE network
  PRIVATE utils
  PRIVATE world
  PRIVATE worldtypes
)

target_include_directories(protocol PUBLIC "export")
