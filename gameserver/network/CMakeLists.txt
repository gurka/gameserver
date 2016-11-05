cmake_minimum_required(VERSION 3.0)

project(network)

set(src
  "src/acceptor.h"
  "src/connection.cc"
  "src/connection.h"
  "src/incomingpacket.cc"
  "src/outgoingpacket.cc"
  "src/server_factory.cc"
  "src/server_impl.cc"
  "src/server_impl.h"
  "export/incomingpacket.h"
  "export/outgoingpacket.h"
  "export/server.h"
  "export/server_factory.h"
)

add_library(network ${src})

if (unittest)
  set(src_test
    "test/acceptor_test.cc"
  )

  add_executable(network_test ${src_test})
  target_link_libraries(network_test
    gtest_main
    gmock_main
    network
    utils_test_stubs
  )

  set_property(TARGET network_test PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
