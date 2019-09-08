cmake_minimum_required(VERSION 3.12)

project(wsclient)
add_executable(wsclient
  "src/graphics.cc"
  "src/graphics.h"
  "src/map.cc"
  "src/map.h"
  "src/network.cc"
  "src/network.h"
  "src/types.h"
  "src/wsclient.cc"
)
target_link_libraries(wsclient
  protocol
  utils
)
