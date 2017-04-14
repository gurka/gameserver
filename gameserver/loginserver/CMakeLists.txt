cmake_minimum_required(VERSION 3.0)

project(loginserver)

set(src
  "src/loginserver.cc"
)

add_executable(loginserver ${src})
target_link_libraries(loginserver
  "account"
  "network"
  "utils"
  ${LIBRARIES}
)
