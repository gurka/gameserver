cmake_minimum_required(VERSION 3.12)

project(gameserver)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# -- Options --
option(GAMESERVER_DEBUG_FULL "Compile with ASAN and clang-tidy" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++17 -pedantic -pthread")

if(GAMESERVER_DEBUG_FULL)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined")
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.*;-warnings-as-errors=*")
endif()

# Use ld.gold if available and if we are using gcc
find_program(LD_GOLD "ld.gold")
if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND LD_GOLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
endif()

# Check for boost (asio + date_time)
find_package(Boost 1.67.0 REQUIRED COMPONENTS system thread date_time)

# -- Binaries --
add_subdirectory("loginserver")
add_subdirectory("worldserver")

# -- Modules --
add_subdirectory("io")
add_subdirectory("protocol")
add_subdirectory("utils")
add_subdirectory("world")
add_subdirectory("gameengine")
add_subdirectory("network")
add_subdirectory("account")

set(CMAKE_CXX_CLANG_TIDY "")

# -- External --
add_library(websocketpp INTERFACE)
target_include_directories(websocketpp SYSTEM INTERFACE "../external/websocketpp")
add_library(rapidxml INTERFACE)
target_include_directories(rapidxml SYSTEM INTERFACE "../external/rapidxml")

# -- Unit tests --
add_subdirectory("../external/googletest" "external/googletest" EXCLUDE_FROM_ALL)
add_subdirectory("account/test" EXCLUDE_FROM_ALL)
add_subdirectory("gameengine/test" EXCLUDE_FROM_ALL)
add_subdirectory("network/test" EXCLUDE_FROM_ALL)
add_subdirectory("utils/test" EXCLUDE_FROM_ALL)
add_subdirectory("world/test" EXCLUDE_FROM_ALL)
add_custom_target(unittest DEPENDS
  account_test
  gameengine_test
  network_test
  utils_test
  world_test
)
