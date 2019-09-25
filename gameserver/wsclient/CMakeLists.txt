cmake_minimum_required(VERSION 3.12)

project(wsclient)

add_executable(wsclient
  "src/graphics.cc"
  "src/graphics.h"
  "src/network.cc"
  "src/network.h"
  "src/types.h"
  "src/wsclient.cc"
  "src/wsworld.cc"
  "src/wsworld.h"
)

target_link_libraries(wsclient
  gameengine
  io
  network
  protocol
  utils
  world
)

set_target_properties(wsclient PROPERTIES OUTPUT_NAME "index.html")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/data.dat" "${CMAKE_BINARY_DIR}/files/data.dat" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../data/sprite.dat" "${CMAKE_BINARY_DIR}/files/sprite.dat" COPYONLY)

target_link_options(wsclient PRIVATE --preload-file "files/")
