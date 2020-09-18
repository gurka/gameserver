cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_library(network_packet
  "export/incoming_packet.h"
  "export/outgoing_packet.h"
  "src/incoming_packet.cc"
  "src/outgoing_packet.cc"
)

target_link_libraries(network_packet PRIVATE
  utils
)

target_include_directories(network_packet PUBLIC "export")

add_library(network_server
  "export/connection.h"
  "export/server_factory.h"
  "export/server.h"
  "src/acceptor.h"
  "src/connection_impl.h"
  "src/server_factory.cc"
  "src/server_impl.h"
  "src/websocket_server_impl.cc"
  "src/websocket_server_impl.h"
)

target_link_libraries(network_server PRIVATE
  network_packet
  utils
  asio
  websocketpp
)

target_include_directories(network_server PUBLIC "export")
