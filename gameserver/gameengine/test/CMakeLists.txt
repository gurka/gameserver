cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(gameengine_test
  "src/container_manager_test.cc"
)

target_include_directories(gameengine_test PRIVATE "../src")

target_link_libraries(gameengine_test PRIVATE
  common
  gameengine
  utils
  world
  gtest_main
  gmock_main
)

target_compile_definitions(gameengine_test PRIVATE UNITTEST)
