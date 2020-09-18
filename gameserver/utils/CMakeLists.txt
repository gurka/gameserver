cmake_minimum_required(VERSION 3.12)

project(utils)

set(FILES
  "export/config_parser.h"
  "export/logger.h"
  "export/tick.h"
  "src/logger.cc"
  "src/tick.cc"
)

# TODO: make Tick emscripten compatible
if (CMAKE_C_COMPILER MATCHES ".*emcc")
  list(FILTER FILES EXCLUDE REGEX ".*/tick.*")
endif()

add_library(utils ${FILES})
target_include_directories(utils PUBLIC "export")
