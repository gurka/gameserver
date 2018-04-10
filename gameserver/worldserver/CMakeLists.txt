cmake_minimum_required(VERSION 3.0)

project(worldserver)

add_executable(worldserver
  "src/protocol_71.cc"
  "src/protocol_71.h"
  "src/protocol.h"
  "src/worldserver.cc"
)

target_link_libraries(worldserver
  account
  network
  gameengine
  world
  utils
  boost_system
  pthread
)
