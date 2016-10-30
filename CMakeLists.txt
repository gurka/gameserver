cmake_minimum_required(VERSION 3.0)

option(gameserver_test "Unit tests" OFF)
project(gameserver)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11 -pedantic -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")


# Required libraries (Boost)
set(LIBRARIES boost_system pthread)


# Loginserver
set(loginserver_src
  "gameserver/loginserver/src/loginserver.cc"
)
set(loginserver_inc
  "gameserver/account/export"
  "gameserver/network/export"
  "gameserver/utils/export"
)
set(loginserver_lib
  "account"
  "network"
  "utils"
)
add_executable(loginserver ${loginserver_src})
target_include_directories(loginserver PUBLIC ${loginserver_inc})
target_link_libraries(loginserver ${loginserver_lib} ${LIBRARIES})


# Worldserver
set(worldserver_src
  "gameserver/worldserver/src/gameengine.cc"
  "gameserver/worldserver/src/gameengine.h"
  "gameserver/worldserver/src/player.cc"
  "gameserver/worldserver/src/playerctrl.cc"
  "gameserver/worldserver/src/playerctrl.h"
  "gameserver/worldserver/src/player.h"
  "gameserver/worldserver/src/taskqueue.h"
  "gameserver/worldserver/src/worldserver.cc"
)
set(worldserver_inc
  "gameserver/account/export"
  "gameserver/network/export"
  "gameserver/utils/export"
  "gameserver/world/export"
)
set(worldserver_lib
  "account"
  "network"
  "utils"
  "world"
)
add_executable(worldserver ${worldserver_src})
target_include_directories(worldserver PUBLIC ${worldserver_inc})
target_link_libraries(worldserver ${worldserver_lib} ${LIBRARIES})


# Utils
set(utils_src
  "gameserver/utils/src/logger.cc"
  "gameserver/utils/export/configparser.h"
  "gameserver/utils/export/logger.h"
)
set(utils_inc
  "gameserver/utils/export"
)
add_library(utils ${utils_src})
target_include_directories(utils PUBLIC ${utils_inc})


# Account
set(account_src
  "gameserver/account/src/account.cc"
  "gameserver/account/export/account.h"
)
set(account_inc
  "gameserver/account/export"
  "gameserver/utils/export"
  "lib/rapidxml"
)
add_library(account ${account_src})
target_include_directories(account PUBLIC ${account_inc})


# Network
set(network_src
  "gameserver/network/src/acceptor.cc"
  "gameserver/network/src/acceptor.h"
  "gameserver/network/src/connection.cc"
  "gameserver/network/src/connection.h"
  "gameserver/network/src/incomingpacket.cc"
  "gameserver/network/src/outgoingpacket.cc"
  "gameserver/network/src/server.cc"
  "gameserver/network/export/incomingpacket.h"
  "gameserver/network/export/outgoingpacket.h"
  "gameserver/network/export/server.h"
)
set(network_inc
  "gameserver/network/src"
  "gameserver/network/export"
  "gameserver/utils/export"
)
add_library(network ${network_src})
target_include_directories(network PUBLIC ${network_inc})


# World
set(world_src
  "gameserver/world/src/creature.cc"
  "gameserver/world/src/item.cc"
  "gameserver/world/src/itemfactory.cc"
  "gameserver/world/src/position.cc"
  "gameserver/world/src/tile.cc"
  "gameserver/world/src/world.cc"
  "gameserver/world/src/worldfactory.cc"
  "gameserver/world/export/creaturectrl.h"
  "gameserver/world/export/creature.h"
  "gameserver/world/export/direction.h"
  "gameserver/world/export/item.h"
  "gameserver/world/export/itemfactory.h"
  "gameserver/world/export/npcctrl.h"
  "gameserver/world/export/position.h"
  "gameserver/world/export/tile.h"
  "gameserver/world/export/world.h"
  "gameserver/world/export/worldfactory.h"
  "gameserver/world/export/worldinterface.h"
)
set(world_inc
  "gameserver/world/src"
  "gameserver/world/export"
  "gameserver/utils/export"
  "lib/rapidxml"
)
add_library(world ${world_src})
target_include_directories(world PUBLIC ${world_inc})


# Unit tests
if (gameserver_test)
  set(unittest_src
    "test/utils/configparser_test.cc"
    "test/account/account_test.cc"
    "test/world/position_test.cc"
    "test/world/creature_test.cc"
    "test/world/item_test.cc"
    "test/world/tile_test.cc"
    "test/world/world_test.cc"
  )

  set(unittest_inc
    "gameserver/account/export"
    "gameserver/network/export"
    "gameserver/utils/export"
    "gameserver/world/export"
    "lib/rapidxml"
  )

  set(unittest_lib
    "account"
    "network"
    "utils"
    "world"
  )

  add_subdirectory(lib/googletest)

  enable_testing()

  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

  add_executable(unittest ${unittest_src})
  target_include_directories(unittest PUBLIC ${unittest_inc})

  target_link_libraries(unittest gtest_main)
  target_link_libraries(unittest gmock_main)
  target_link_libraries(unittest ${unittest_lib})
endif()
