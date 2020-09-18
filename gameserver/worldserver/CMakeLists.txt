cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(worldserver
  "src/connection_ctrl.cc"
  "src/connection_ctrl.h"
  "src/worldserver.cc"
)

target_link_libraries(worldserver PRIVATE
  account
  common
  gameengine
  network_server
  protocol_server
  world
  utils
  asio
)
