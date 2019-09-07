cmake_minimum_required(VERSION 3.13)

project(protocol)
add_library(protocol
  "export/protocol.h"
  "export/protocol_helper.h"
  "src/protocol.cc"
  "src/protocol_helper.cc"
)
target_include_directories(protocol PUBLIC "export")
target_link_libraries(protocol
  account
  gameengine
  network
  utils
  world
)
