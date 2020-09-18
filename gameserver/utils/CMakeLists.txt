cmake_minimum_required(VERSION 3.12)

project(gameserver)

set(FILES
  "export/config_parser.h"
  "export/data_loader.h"
  "export/file_reader.h"
  "export/logger.h"
  "export/tick.h"
  "src/data_loader.cc"
  "src/logger.cc"
  "src/tick.cc"
)

add_library(utils ${FILES})

target_link_libraries(utils PRIVATE
  common
  rapidxml
)

target_include_directories(utils PUBLIC "export")
