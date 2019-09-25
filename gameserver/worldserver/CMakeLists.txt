cmake_minimum_required(VERSION 3.12)

project(worldserver)

add_executable(worldserver
  "src/connection_ctrl.cc"
  "src/connection_ctrl.h"
  "src/worldserver.cc"
)

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

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/loginserver.cfg" "${CMAKE_BINARY_DIR}/data/loginserver.cfg" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/worldserver.cfg" "${CMAKE_BINARY_DIR}/data/worldserver.cfg" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/accounts.xml"    "${CMAKE_BINARY_DIR}/data/accounts.xml"    COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/data.dat"        "${CMAKE_BINARY_DIR}/data/data.dat"        COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/items.xml"       "${CMAKE_BINARY_DIR}/data/items.xml"       COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/world.xml"       "${CMAKE_BINARY_DIR}/data/world.xml"       COPYONLY)
