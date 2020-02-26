cmake_minimum_required(VERSION 3.12)

project(protocol)

add_library(protocol
  "export/protocol_client.h"
  "export/protocol_common.h"
  "export/protocol_server.h"
  "src/protocol_client.cc"
  "src/protocol_common.cc"
  "src/protocol_server.cc"
)

target_link_libraries(protocol PRIVATE
  common
  gameengine
  network
  utils
  world
)

target_include_directories(protocol PUBLIC "export")
