cmake_minimum_required(VERSION 3.0)

project(gameengine_test)

add_executable(gameengine_test
  "src/container_manager_test.cc"
)

target_link_libraries(gameengine_test
  gameengine
  utils
  world
  gtest_main
  gmock_main
)
