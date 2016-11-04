cmake_minimum_required(VERSION 3.0)

option(unittest "Unit tests" OFF)
project(gameserver)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11 -pedantic -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")


# Required libraries (Boost)
set(LIBRARIES boost_system pthread)

if (NOT unittest)
  # Loginserver
  add_subdirectory("gameserver/loginserver")
  target_include_directories(loginserver PUBLIC
    "gameserver/account/export"
    "gameserver/network/export"
    "gameserver/utils/export"
  )

  # Worldserver
  add_subdirectory("gameserver/worldserver")
  target_include_directories(worldserver PUBLIC
    "gameserver/account/export"
    "gameserver/network/export"
    "gameserver/utils/export"
    "gameserver/world/export"
  )
endif()

# Account
add_subdirectory("gameserver/account")
target_include_directories(account PUBLIC
  "gameserver/account/export"
  "gameserver/utils/export"
  "lib/rapidxml"
)

# Utils
add_subdirectory("gameserver/utils")
target_include_directories(utils PUBLIC
  "gameserver/utils/export"
)

# Network
add_subdirectory("gameserver/network")
target_include_directories(network PUBLIC
  "gameserver/network/src"
  "gameserver/network/export"
  "gameserver/utils/export"
)

# World
add_subdirectory("gameserver/world")
target_include_directories(world PUBLIC
  "gameserver/world/src"
  "gameserver/world/export"
  "gameserver/utils/export"
  "lib/rapidxml"
)

# Unit tests
if (unittest)
  add_subdirectory(lib/googletest)

  enable_testing()

  target_include_directories(test_account PUBLIC ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  target_include_directories(test_account PUBLIC ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

  target_include_directories(test_utils PUBLIC ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  target_include_directories(test_utils PUBLIC ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

  #target_include_directories(test_network PUBLIC ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  #target_include_directories(test_network PUBLIC ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

  target_include_directories(test_world PUBLIC ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  target_include_directories(test_world PUBLIC ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})
endif()
