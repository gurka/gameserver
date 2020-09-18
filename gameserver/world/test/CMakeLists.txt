cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(world_test
  "src/creaturectrl_mock.h"
  "src/world_test.cc"
  "src/tile_test.cc"
)

target_link_libraries(world_test PRIVATE
  common
  world
  utils
  gtest_main
  gmock_main
)

target_compile_definitions(world_test PRIVATE UNITTEST)
