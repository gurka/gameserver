cmake_minimum_required(VERSION 3.12)

project(io)

add_library(io
  "export/account_loader.h"
  "export/data_loader.h"
  "export/sprite_loader.h"
  "export/world_loader.h"
  "src/account_loader.cc"
  "src/data_loader.cc"
  "src/file_reader.h"
  "src/sprite_loader.cc"
  "src/world_loader.cc"
)

target_link_libraries(io
  PRIVATE account
  PRIVATE utils
  PRIVATE world
  PRIVATE rapidxml
)

target_include_directories(io PUBLIC "export")
