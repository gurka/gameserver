cmake_minimum_required(VERSION 3.0)

project(network_test)

add_executable(network_test
  "src/acceptor_test.cc"
  "src/backend_mock.h"
  "src/connection_test.cc"
  "src/server_test.cc"
  "src/packet_test.cc"
)

target_link_libraries(network_test
  network
  utils
  gtest_main
  gmock_main
)
