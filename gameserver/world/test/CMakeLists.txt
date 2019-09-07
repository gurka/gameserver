cmake_minimum_required(VERSION 3.0)

project(world_test)
add_executable(world_test
  "src/position_test.cc"
  "src/creaturectrl_mock.h"
  "src/creature_test.cc"
  "src/world_test.cc"
  "src/tile_test.cc"
)
target_link_libraries(world_test
  world
  utils
  gtest_main
  gmock_main
)
target_compile_definitions(world_test PRIVATE UNITTEST)
set_target_properties(world_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
