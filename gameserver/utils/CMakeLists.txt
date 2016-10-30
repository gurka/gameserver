cmake_minimum_required(VERSION 3.0)

project(utils)

set(src
  "src/logger.cc"
  "export/configparser.h"
  "export/logger.h"
)

add_library(utils ${src})
