cmake_minimum_required(VERSION 3.12)

project(protocol)
add_library(protocol
  "export/protocol_helper.h"
  "export/protocol_types.h"
  "src/protocol_helper.cc"
)
target_link_libraries(protocol
  gameengine
  network
  utils
  world
)
target_include_directories(protocol PUBLIC "export")
