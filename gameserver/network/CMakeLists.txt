cmake_minimum_required(VERSION 3.0)

project(network)

set(src
  "src/acceptor.cc"
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
  #set(test_src
  #)
  #
  #add_executable(test_network ${test_src})
  #target_link_libraries(test_network
  #  gtest_main
  #  gmock_main
  #)
  #
  #set_property(TARGET test_network PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
