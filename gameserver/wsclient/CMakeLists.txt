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

target_link_libraries(wsclient PRIVATE
  gameengine
  io
  network
  protocol
  utils
  world
)
