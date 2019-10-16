cmake_minimum_required(VERSION 3.12)

project(wsclient)

add_executable(wsclient
  "src/graphics.cc"
  "src/graphics.h"
  "src/network.cc"
  "src/network.h"
  "src/texture.cc"
  "src/texture.h"
  "src/types.h"
  "src/wsclient.cc"
  "src/wsworld.cc"
  "src/wsworld.h"
)

target_link_libraries(wsclient PRIVATE
  common
  gameengine
  io
  network
  protocol
  utils
)

set_target_properties(wsclient PROPERTIES OUTPUT_NAME "index.html")

target_link_options(wsclient PRIVATE --preload-file "../files/")

add_executable(itemview
  "src/itemview.cc"
  "src/texture.cc"
  "src/texture.h"
)

target_link_libraries(itemview PRIVATE
  common
  io
  utils
)

set_target_properties(itemview PROPERTIES OUTPUT_NAME "itemview.html")

target_link_options(itemview PRIVATE --preload-file "../files/")
