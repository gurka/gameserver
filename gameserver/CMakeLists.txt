cmake_minimum_required(VERSION 3.0)

option(unittest "Unit tests" OFF)
project(gameserver)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11 -pedantic -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# Required libraries (Boost)
set(LIBRARIES boost_system pthread)

if (NOT unittest)
  # Loginserver
  add_subdirectory("loginserver")
  target_include_directories(loginserver PUBLIC
    "account/export"
    "network/export"
    "utils/export"
  )

  # Worldserver
  add_subdirectory("worldserver")
  target_include_directories(worldserver PUBLIC
    "account/export"
    "network/export"
    "utils/export"
    "world/export"
    "gameengine/export"
  )
endif()

# Account
add_subdirectory("account")
target_include_directories(account PUBLIC
  "account/export"
  "utils/export"
  "external/rapidxml"
)

# Utils
add_subdirectory("utils")
target_include_directories(utils PUBLIC
  "utils/export"
)

# Network
add_subdirectory("network")
target_include_directories(network PUBLIC
  "network/src"
  "network/export"
  "utils/export"
)

# World
add_subdirectory("world")
target_include_directories(world PUBLIC
  "world/src"
  "world/export"
  "utils/export"
  "external/rapidxml"
)

# Gameengine
add_subdirectory("gameengine")
target_include_directories(gameengine PUBLIC
  "gameengine/src"
  "gameengine/export"
  "world/export"
  "utils/export"
)

# Unit tests
if (unittest)
  add_subdirectory(external/googletest)

  enable_testing()

  # Utils stub
  target_include_directories(utils_test_stubs PUBLIC
    "utils/export"
  )

  # Account test
  target_include_directories(account_test PUBLIC
    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
    ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
  )

  # Utils test
  target_include_directories(utils_test PUBLIC
    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
    ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
  )

  # Network test
  target_include_directories(network_test PUBLIC
    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
    ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
  )

  # World test
  target_include_directories(world_test PUBLIC
    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
    ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
  )
endif()
