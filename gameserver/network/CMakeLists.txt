cmake_minimum_required(VERSION 3.0)

project(network)

set(src
  "src/acceptor.cc"
  "src/acceptor.h"
  "src/connection.cc"
  "src/connection.h"
  "src/incomingpacket.cc"
  "src/outgoingpacket.cc"
  "src/server.cc"
  "export/incomingpacket.h"
  "export/outgoingpacket.h"
  "export/server.h"
)

add_library(network ${src})
