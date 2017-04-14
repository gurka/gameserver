cmake_minimum_required(VERSION 3.0)

project(utils)

set(src
  "src/logger.cc"
  "export/configparser.h"
  "export/logger.h"
  "export/taskqueue.h"
  "export/taskqueuefactory.h"
  "export/tick.h"
  "src/taskqueueimpl.cc"
  "src/taskqueueimpl.h"
  "src/taskqueuefactory.cc"
  "src/tick.cc"
)

add_library(utils ${src})

if (unittest)
  # Export / stubs
  set(src_test_stubs
    "test_export/stub_logger.cc"
    "test_export/stub_tick.cc"
  )
  add_library(utils_test_stubs ${src_test_stubs})

  # Unit test
  set(src_test
    "test/configparser_test.cc"
  )

  add_executable(utils_test ${src_test})
  target_link_libraries(utils_test
    gtest_main
    gmock_main
    utils
  )
endif()
