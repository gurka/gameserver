cmake_minimum_required(VERSION 3.12)

project(utils)
if(NOT GAMESERVER_WSCLIENT)
  add_library(utils
    "export/config_parser.h"
    "export/logger.h"
    "export/tick.h"
    "src/logger.cc"
    "src/tick.cc"
  )
else()
  add_library(utils
    "export/logger.h"
    "src/logger.cc"
  )
endif()

target_include_directories(utils PUBLIC "export")
