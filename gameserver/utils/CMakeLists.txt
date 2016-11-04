cmake_minimum_required(VERSION 3.0)

project(utils)

set(src
  "src/logger.cc"
  "export/configparser.h"
  "export/logger.h"
)

add_library(utils ${src})

if (unittest)
  set(test_src
    "test/configparser_test.cc"
  )

  add_executable(test_utils ${test_src})
  target_link_libraries(test_utils
    gtest_main
    gmock_main
    utils
  )

  set_property(TARGET test_utils PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
