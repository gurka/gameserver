cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(utils_test
  "src/configparser_test.cc"
)

target_link_libraries(utils_test PRIVATE
  utils
  gtest_main
  gmock_main
)

target_compile_definitions(utils_test PRIVATE UNITTEST)
