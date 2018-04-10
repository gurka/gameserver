cmake_minimum_required(VERSION 3.0)

project(utils)

add_library(utils
  "export/config_parser.h"
  "export/logger.h"
  "export/tick.h"
  "src/logger.cc"
  "src/tick.cc"
)
