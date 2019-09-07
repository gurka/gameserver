cmake_minimum_required(VERSION 3.12)

project(worldserver)
add_executable(worldserver "src/worldserver.cc")
target_link_libraries(worldserver
  account
  gameengine
  network
  protocol
  world
  utils
  boost_system
  pthread
)
