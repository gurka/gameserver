cmake_minimum_required(VERSION 3.12)

project(io)
add_library(io
  "export/data_loader.h"
  "export/sprite_loader.h"
  "src/data_loader.cc"
  "src/file_reader.h"
  "src/sprite_loader.cc"
)
target_link_libraries(io
  world
)
target_include_directories(io PUBLIC "export")
