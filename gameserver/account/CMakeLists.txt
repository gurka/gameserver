cmake_minimum_required(VERSION 3.0)

project(account)

set(src
  "src/account.cc"
  "export/account.h"
)

add_library(account ${src})
