cmake_minimum_required(VERSION 3.0)

project(world)

set(src
  "src/creature.cc"
  "src/item.cc"
  "src/itemfactory.cc"
  "src/position.cc"
  "src/tile.cc"
  "src/world.cc"
  "src/worldfactory.cc"
  "export/creaturectrl.h"
  "export/creature.h"
  "export/direction.h"
  "export/item.h"
  "export/itemfactory.h"
  "export/npcctrl.h"
  "export/position.h"
  "export/tile.h"
  "export/world.h"
  "export/worldfactory.h"
  "export/worldinterface.h"
)

add_library(world ${src})

if (unittest)
  set(test_src
    "test/mocks/creaturectrl_mock.h"
    "test/mocks/itemfactory_mock.h"
    "test/position_test.cc"
    "test/creature_test.cc"
    "test/item_test.cc"
    "test/world_test.cc"
    "test/tile_test.cc"
  )

  add_executable(test_world ${test_src})
  target_link_libraries(test_world
    gtest_main
    gmock_main
    world
    utils
  )

  set_property(TARGET test_world PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
