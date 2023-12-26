cmake_minimum_required(VERSION 3.12)

project(gameserver)

set(FILES
  "export/utils/config_parser.h"
  "export/utils/data_loader.h"
  "export/utils/file_reader.h"
  "export/utils/logger.h"
  "export/utils/tick.h"
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
