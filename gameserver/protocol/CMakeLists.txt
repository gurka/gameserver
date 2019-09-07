cmake_minimum_required(VERSION 3.0)

project(protocol)
add_library(protocol
  "export/protocol.h"
  "export/protocol_71.h"
  "src/protocol_71.cc"
)
target_include_directories(protocol PUBLIC "export")
target_link_libraries(protocol
  account
  gameengine
  network
  utils
  world
)
