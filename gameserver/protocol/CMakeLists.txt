cmake_minimum_required(VERSION 3.12)

project(protocol)
if(NOT GAMESERVER_WSCLIENT)
  add_library(protocol
    "export/protocol.h"
    "export/protocol_helper.h"
    "export/protocol_types.h"
    "src/protocol.cc"
    "src/protocol_helper.cc"
  )
  target_link_libraries(protocol
    account
    gameengine
    network
    utils
    world
  )
else()
  add_library(protocol
    "export/protocol_helper.h"
    "export/protocol_types.h"
    "src/protocol_helper.cc"
  )
  target_link_libraries(protocol
    gameengine
    network
    utils
    world
  )
endif()
target_include_directories(protocol PUBLIC "export")
