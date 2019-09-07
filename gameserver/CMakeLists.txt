cmake_minimum_required(VERSION 3.12)

project(gameserver)

# -- Options --
option(GAMESERVER_WSCLIENT "Build wsclient" OFF)
option(GAMESERVER_USE_LD_GOLD "Pass -fuse-ld=gold to compiler" OFF)
option(GAMESERVER_USE_ASAN "Compile with ASAN" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(NOT GAMESERVER_WSCLIENT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++14 -pedantic -pthread")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -std=c++14 --bind")
endif()

if(GAMESERVER_USE_LD_GOLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
endif()

if(GAMESERVER_USE_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined")
endif()

# -- Binaries --
if(NOT GAMESERVER_WSCLIENT)
  add_subdirectory("loginserver")
  add_subdirectory("worldserver")
else()
  add_subdirectory("wsclient")
  set_target_properties(wsclient PROPERTIES OUTPUT_NAME "index.html")
endif()

# -- Modules --
add_subdirectory("protocol")
add_subdirectory("utils")
add_subdirectory("world")
add_subdirectory("gameengine")
add_subdirectory("network")
if(NOT GAMESERVER_WSCLIENT)
  add_subdirectory("account")
endif()

# -- External --
if(NOT GAMESERVER_WSCLIENT)
  add_library(websocketpp INTERFACE)
  target_include_directories(websocketpp INTERFACE "../external/websocketpp")
  add_library(rapidxml INTERFACE)
  target_include_directories(rapidxml INTERFACE "../external/rapidxml")
endif()

# -- Unit tests --
if(NOT GAMESERVER_WSCLIENT)
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
endif()
