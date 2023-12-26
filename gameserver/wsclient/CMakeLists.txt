cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(replayclient
  "src/console.cc"
  "src/console.h"
  "src/graphics.cc"
  "src/graphics.h"
  "src/player_info.h"
  "src/replay_client.cc"
  "src/replay_reader.cc"
  "src/replay_reader.h"
  "src/sprite_loader.cc"
  "src/sprite_loader.h"
  "src/texture.cc"
  "src/texture.h"
  "src/tiles.cc"
  "src/tiles.h"
  "src/types.h"
  "src/map.cc"
  "src/wsworld.h"
)

target_link_libraries(replayclient PRIVATE
  common
  network_client
  protocol_client
  utils
)

if (EMSCRIPTEN)
  set_target_properties(replayclient PROPERTIES SUFFIX ".html")
  target_link_options(replayclient PRIVATE --preload-file "../files/")
else()
  target_link_libraries(replayclient PRIVATE
    asio
    SDL2
    SDL2_ttf
  )
endif()
