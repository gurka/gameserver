cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_library(account
  "export/account.h"
  "src/account.cc"
  "src/loader.cc"
  "src/loader.h"
)

target_include_directories(account PUBLIC "export")

target_link_libraries(account PRIVATE
  utils
  rapidxml
)
