cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(replayclient
  "src/game/game.cc"
  "src/game/game.h"
  "src/game/game_ui.cc"
  "src/game/game_ui.h"
  "src/game/sprite_loader.cc"
  "src/game/sprite_loader.h"
  "src/game/texture.cc"
  "src/game/texture.h"
  "src/game/tiles.cc"
  "src/game/tiles.h"
  "src/game/types.h"
  "src/main_ui.cc"
  "src/main_ui.h"
  "src/player_info.h"
  "src/protocol.cc"
  "src/protocol.h"
  "src/replay_client.cc"
  "src/replay_reader.cc"
  "src/replay_reader.h"
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
