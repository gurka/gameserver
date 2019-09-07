cmake_minimum_required(VERSION 3.12)

project(wsclient)
add_executable(wsclient
  "src/wsclient.cc"
  "src/network.cc"
  "src/network.h")
target_link_libraries(wsclient
  protocol
  utils
)
