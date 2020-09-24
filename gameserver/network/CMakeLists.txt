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
  "src/websocketpp_server_backend.cc"
  "src/websocketpp_server_backend.h"
)

target_link_libraries(network_server PRIVATE
  network_packet
  utils
  asio
  websocketpp
)

target_include_directories(network_server PUBLIC "export")

if(NOT EMSCRIPTEN)
  add_library(network_client
    "export/client_factory.h"
    "src/websocketpp_client_backend.cc"
    "src/websocketpp_client_backend.h"
    "src/websocketpp_client_factory.cc"
  )
  
  target_link_libraries(network_client PRIVATE
    network_packet
    utils
    asio
    websocketpp
  )
else()
  add_library(network_client
    "export/client_factory.h"
    "src/emscripten_client_backend.cc"
    "src/emscripten_client_backend.h"
    "src/emscripten_client_factory.cc"
  )
  
  target_link_libraries(network_client PRIVATE
    network_packet
    utils
  )
endif()

target_include_directories(network_client PUBLIC "export")
