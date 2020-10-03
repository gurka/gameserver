cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(wsclient
  "src/graphics.cc"
  "src/graphics.h"
  "src/sprite_loader.cc"
  "src/sprite_loader.h"
  "src/texture.cc"
  "src/texture.h"
  "src/tiles.cc"
  "src/tiles.h"
  "src/types.h"
  "src/wsclient.cc"
  "src/wsworld.cc"
  "src/wsworld.h"
)

target_link_libraries(wsclient PRIVATE
  common
  network_client
  protocol_client
  utils
)

if (EMSCRIPTEN)
  set_target_properties(wsclient PROPERTIES SUFFIX ".html")
  target_link_options(wsclient PRIVATE --preload-file "../files/")
else()
  target_link_libraries(wsclient PRIVATE
    SDL2
    asio
    websocketpp
  )
endif()

add_executable(itemview
  "src/itemview.cc"
  "src/sprite_loader.cc"
  "src/sprite_loader.h"
  "src/texture.cc"
  "src/texture.h"
)

target_link_libraries(itemview PRIVATE
  common
  utils
)

if (EMSCRIPTEN)
  set_target_properties(itemview PROPERTIES SUFFIX ".html")
  target_link_options(itemview PRIVATE --preload-file "../files/")
else()
  target_link_libraries(itemview PRIVATE
    SDL2
    asio
  )
endif()