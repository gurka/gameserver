cmake_minimum_required(VERSION 3.0)

project(loginserver)

add_executable(loginserver
  "src/loginserver.cc"
)

target_link_libraries(loginserver
  account
  network
  utils
  boost_system
  pthread
)
