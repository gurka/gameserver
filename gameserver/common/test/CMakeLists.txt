cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(common_test
  "src/position_test.cc"
  "src/creature_test.cc"
)

target_link_libraries(common_test PRIVATE
  common
  utils
  gtest_main
  gmock_main
)

target_compile_definitions(common_test PRIVATE UNITTEST)
