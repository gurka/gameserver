cmake_minimum_required(VERSION 3.0)

project(account_test)

add_executable(account_test
  "src/account_test.cc"
)

target_link_libraries(account_test
  account
  utils
  gtest_main
  gmock_main
)

target_compile_definitions(account_test PRIVATE UNITTEST)
set_target_properties(account_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
