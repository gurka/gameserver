cmake_minimum_required(VERSION 3.12)

project(worldserver)

add_executable(worldserver
  "src/connection_ctrl.cc"
  "src/connection_ctrl.h"
  "src/worldserver.cc"
)

target_link_libraries(worldserver PRIVATE
  account
  gameengine
  network
  protocol
  world
  worldtypes
  utils
  boost_system
  pthread
)
