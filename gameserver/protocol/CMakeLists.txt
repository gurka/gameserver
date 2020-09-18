cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_library(protocol_common
  "export/protocol_common.h"
  "src/protocol_common.cc"
)

target_link_libraries(protocol_common PRIVATE
  common
  network_packet
  utils
)

target_include_directories(protocol_common PUBLIC "export")

add_library(protocol_client
  "export/protocol_client.h"
  "src/protocol_client.cc"
)

target_link_libraries(protocol_client PRIVATE
  protocol_common
  common
  network_packet
  utils
)

target_include_directories(protocol_client PUBLIC "export")

add_library(protocol_server
  "export/protocol_server.h"
  "src/protocol_server.cc"
)

target_link_libraries(protocol_server PRIVATE
  protocol_common
  common
  gameengine
  network_packet
  utils
  world
)

target_include_directories(protocol_server PUBLIC "export")
