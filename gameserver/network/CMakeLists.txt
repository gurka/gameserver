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
