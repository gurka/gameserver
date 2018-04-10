cmake_minimum_required(VERSION 3.0)

project(utils_test)

add_executable(utils_test
  "src/configparser_test.cc"
)

target_link_libraries(utils_test
  utils
  gtest_main
  gmock_main
)
