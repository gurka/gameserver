cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(wsclient_test
  "src/tiles_test.cc"
)

target_link_libraries(wsclient_test PRIVATE
  common
  utils
  gtest_main
  gmock_main
)

target_compile_definitions(wsclient_test PRIVATE UNITTEST)
