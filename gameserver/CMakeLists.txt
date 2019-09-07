cmake_minimum_required(VERSION 3.0)

project(gameserver)

# -- Options --
option(GAMESERVER_USE_LD_GOLD "Pass -fuse-ld=gold to compiler" OFF)
option(GAMESERVER_USE_ASAN "Compile with ASAN" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++14 -pedantic -pthread")

if(GAMESERVER_USE_LD_GOLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
endif()

if(GAMESERVER_USE_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined")
endif()

# -- Binaries --

add_subdirectory("loginserver")
target_include_directories(loginserver PUBLIC
  "account/export"
  "network/export"
  "utils/export"
)

add_subdirectory("worldserver")
target_include_directories(worldserver PUBLIC
  "account/export"
  "network/export"
  "utils/export"
  "world/export"
)

# -- Modules --

add_subdirectory("account")
target_include_directories(account PUBLIC
  "account/export"
  "utils/export"
)
target_include_directories(account SYSTEM PRIVATE
  "../external/rapidxml"
)

add_subdirectory("utils")
target_include_directories(utils PUBLIC
  "utils/export"
)

add_subdirectory("network")
target_include_directories(network PUBLIC
  "network/export"
  "utils/export"
)

add_subdirectory("gameengine")
target_include_directories(gameengine PUBLIC
  "gameengine/export"
  "utils/export"
  "world/export"
)
target_include_directories(gameengine SYSTEM PRIVATE
  "../external/rapidxml"
)

add_subdirectory("world")
target_include_directories(world PUBLIC
  "world/export"
  "utils/export"
)

# -- Unit tests --

# Google Test
add_subdirectory(../external/googletest external/googletest)
set_target_properties(gtest PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gmock PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gtest_main PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gmock_main PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Account test
add_subdirectory("account/test")
target_include_directories(account_test PUBLIC
  "account/export"
  "account/src"
)
target_include_directories(account_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

# Gameengine test
add_subdirectory("gameengine/test")
target_include_directories(gameengine_test PUBLIC
  "gameengine/export"
  "gameengine/src"
)
target_include_directories(gameengine_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

# Network test
add_subdirectory("network/test")
target_include_directories(network_test PUBLIC
  "network/export"
  "network/src"
)
target_include_directories(network_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

# Utils test
add_subdirectory("utils/test")
target_include_directories(utils_test PUBLIC
  "utils/export"
  "utils/src"
)
target_include_directories(utils_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

# World test
add_subdirectory("world/test")
target_include_directories(world_test PUBLIC
  "world/export"
  "world/src"
)
target_include_directories(world_test SYSTEM PRIVATE
  ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
  ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
)

# Build all tests with target 'unittest'
add_custom_target(unittest DEPENDS
  account_test
  gameengine_test
  network_test
  utils_test
  world_test
)
