cmake_minimum_required(VERSION 3.12)

project(account)
add_library(account
  "export/account.h"
  "src/account.cc"
)
target_include_directories(account PUBLIC "export")
target_link_libraries(account
  io
  utils
)
