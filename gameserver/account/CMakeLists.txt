cmake_minimum_required(VERSION 3.0)

project(account)

add_library(account
  "export/account.h"
  "src/account.cc"
)
