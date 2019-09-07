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
add_subdirectory("worldserver")

# -- Modules --
add_subdirectory("account")
add_subdirectory("utils")
add_subdirectory("network")
add_subdirectory("protocol")
add_subdirectory("gameengine")
add_subdirectory("world")

# -- External --
add_library(rapidxml INTERFACE)
target_include_directories(rapidxml INTERFACE "../external/rapidxml")

# -- Unit tests --
add_subdirectory(../external/googletest external/googletest)
set_target_properties(gtest PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gmock PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gtest_main PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(gmock_main PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_subdirectory("account/test")
add_subdirectory("gameengine/test")
add_subdirectory("network/test")
add_subdirectory("utils/test")
add_subdirectory("world/test")

add_custom_target(unittest DEPENDS
  account_test
  gameengine_test
  network_test
  utils_test
  world_test
)
