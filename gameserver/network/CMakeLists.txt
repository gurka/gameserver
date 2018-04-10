cmake_minimum_required(VERSION 3.0)

project(network)

add_library(network
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
