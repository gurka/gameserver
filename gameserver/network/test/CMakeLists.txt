cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(network_test
  "src/acceptor_test.cc"
  "src/backend_mock.h"
  "src/connection_test.cc"
  "src/server_test.cc"
  "src/packet_test.cc"
)

target_include_directories(network_test PRIVATE "../src")

target_link_libraries(network_test PRIVATE
  network_server
  network_packet
  utils
  gtest_main
  gmock_main
)

target_compile_definitions(network_test PRIVATE UNITTEST)
