cmake_minimum_required(VERSION 3.0)

project(account)

set(src
  "src/account.cc"
  "export/account.h"
)

add_library(account ${src})

if (unittest)
  set(test_src
    "test/account_test.cc"
  )

  add_executable(test_account ${test_src})
  target_link_libraries(test_account
    gtest_main
    gmock_main
    account
    utils
  )

  set_property(TARGET test_account PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
