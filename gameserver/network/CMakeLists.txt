cmake_minimum_required(VERSION 3.12)

project(network)
if(NOT GAMESERVER_WSCLIENT)
  add_library(network
    "export/connection.h"
    "export/incoming_packet.h"
    "export/outgoing_packet.h"
    "export/server_factory.h"
    "export/server.h"
    "src/acceptor.h"
    "src/connection_impl.h"
    "src/incoming_packet.cc"
    "src/outgoing_packet.cc"
    "src/server_factory.cc"
    "src/server_impl.h"
    "src/websocket_server_impl.cc"
    "src/websocket_server_impl.h"
  )
else()
  add_library(network
    "export/incoming_packet.h"
    "export/outgoing_packet.h"
    "src/incoming_packet.cc"
    "src/outgoing_packet.cc"
  )
endif()
target_include_directories(network PUBLIC "export")
target_link_libraries(network
  utils
  websocketpp
)
