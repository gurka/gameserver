cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(replayserver
  "src/replay_connection.cc"
  "src/replay_connection.h"
  "src/replay_reader.cc"
  "src/replay_reader.h"
  "src/replayserver.cc"
)

target_link_libraries(replayserver PRIVATE
  network_server
  utils
  asio
)
