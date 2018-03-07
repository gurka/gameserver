cmake_minimum_required(VERSION 3.0)

project(account)

set(src
  "export/account.h"
  "src/account.cc"
)

add_library(account ${src})

if (unittest)
  set(src_test
    "test/account_test.cc"
  )

  add_executable(account_test ${src_test})
  target_link_libraries(account_test
    gtest_main
    gmock_main
    account
    utils_test_stubs
  )
endif()
