cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(replayclient
  "src/chat.cc"
  "src/chat.h"
  "src/chat_ui.cc"
  "src/chat_ui.h"
  "src/game.cc"
  "src/game.h"
  "src/game_ui.cc"
  "src/game_ui.h"
  "src/main_ui.cc"
  "src/main_ui.h"
  "src/player_info.h"
  "src/protocol.cc"
  "src/protocol.h"
  "src/replay_client.cc"
  "src/replay_reader.cc"
  "src/replay_reader.h"
  "src/sidebar.cc"
  "src/sidebar.h"
  "src/sidebar_ui.cc"
  "src/sidebar_ui.h"
  "src/sprite_loader.cc"
  "src/sprite_loader.h"
  "src/texture.cc"
  "src/texture.h"
  "src/tiles.cc"
  "src/tiles.h"
  "src/types.h"
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
