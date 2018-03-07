cmake_minimum_required(VERSION 3.0)

project(network)

set(src
  "export/incoming_packet.h"
  "export/outgoing_packet.h"
  "export/server_factory.h"
  "export/server.h"
  "src/acceptor.h"
  "src/connection.h"
  "src/incoming_packet.cc"
  "src/outgoing_packet.cc"
  "src/server_factory.cc"
  "src/server_impl.h"
)

add_library(network ${src})

if (unittest)
  set(src_test
    "test/acceptor_test.cc"
    "test/connection_test.cc"
    "test/server_test.cc"
    "test/packet_test.cc"
  )

  add_executable(network_test ${src_test})
  target_link_libraries(network_test
    gtest_main
    gmock_main
    network
    utils_test_stubs
  )
endif()
