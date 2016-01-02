cmake_minimum_required(VERSION 3.0)

option(gameserver_test "Unit tests" OFF)
project(gameserver)


## Settings
# modules compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11 -pedantic -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# Required libraries (Boost)
set(LIBRARIES boost_system pthread)


## Executables
# Loginserver
set(loginserver_src
  "src/loginserver/loginserver.cc"
)
set(loginserver_inc
  "src/account"
  "src/network"
  "src/utils"
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
  "src/worldserver/gameengine.cc"
  "src/worldserver/gameengine.h"
  "src/worldserver/player.cc"
  "src/worldserver/playerctrl.cc"
  "src/worldserver/playerctrl.h"
  "src/worldserver/player.h"
  "src/worldserver/taskqueue.h"
  "src/worldserver/worldserver.cc"
)
set(worldserver_inc
  "src/account"
  "src/network"
  "src/utils"
  "src/world"
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


## Libraries
# Utils
set(utils_src
  "src/utils/configparser.h"
  "src/utils/logger.cc"
  "src/utils/logger.h"
)
add_library(utils ${utils_src})

# Account
set(account_src
  "src/account/account.cc"
  "src/account/account.h"
)
set(account_inc
  "src/utils"
  "lib/rapidxml"
)
add_library(account ${account_src})
target_include_directories(account PUBLIC ${account_inc})

# Network
set(network_src
  "src/network/acceptor.cc"
  "src/network/acceptor.h"
  "src/network/connection.cc"
  "src/network/connection.h"
  "src/network/incomingpacket.cc"
  "src/network/incomingpacket.h"
  "src/network/outgoingpacket.cc"
  "src/network/outgoingpacket.h"
  "src/network/server.cc"
  "src/network/server.h"
)
set(network_inc
  "src/utils"
)
add_library(network ${network_src})
target_include_directories(network PUBLIC ${network_inc})

# World
set(world_src
  "src/world/creature.cc"
  "src/world/creaturectrl.h"
  "src/world/creature.h"
  "src/world/direction.h"
  "src/world/item.cc"
  "src/world/item.h"
  "src/world/itemfactory.cc"
  "src/world/itemfactory.h"
  "src/world/npcctrl.h"
  "src/world/position.cc"
  "src/world/position.h"
  "src/world/tile.cc"
  "src/world/tile.h"
  "src/world/world.cc"
  "src/world/world.h"
  "src/world/worldfactory.cc"
  "src/world/worldfactory.h"
  "src/world/worldinterface.h"
)
set(world_inc
  "src/utils"
  "lib/rapidxml"
)
add_library(world ${world_src})
target_include_directories(world PUBLIC ${world_inc})

## Unit tests
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
    "src/account"
    "src/network"
    "src/utils"
    "src/world"
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
