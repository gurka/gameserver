cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(account_test
  "src/account_test.cc"
)

target_link_libraries(account_test PRIVATE
  account
  utils
  gtest_main
  gmock_main
)

target_compile_definitions(account_test PRIVATE UNITTEST)
