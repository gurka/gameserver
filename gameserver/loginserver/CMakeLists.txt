cmake_minimum_required(VERSION 3.12)

project(loginserver)

add_executable(loginserver "src/loginserver.cc")

target_link_libraries(loginserver PRIVATE
  account
  network
  utils
  pthread
)
