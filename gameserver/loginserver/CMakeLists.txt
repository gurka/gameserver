cmake_minimum_required(VERSION 3.12)

project(gameserver)

add_executable(loginserver "src/loginserver.cc")

target_link_libraries(loginserver PRIVATE
  account
  network_server
  utils
  asio
)
