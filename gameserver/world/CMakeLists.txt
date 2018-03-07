cmake_minimum_required(VERSION 3.0)

project(world)

set(src
  "export/creature_ctrl.h"
  "export/creature.h"
  "export/direction.h"
  "export/item_factory.h"
  "export/item.h"
  "export/position.h"
  "export/tile.h"
  "export/world_factory.h"
  "export/world_interface.h"
  "export/world.h"
  "src/creature.cc"
  "src/item_factory.cc"
  "src/item.cc"
  "src/position.cc"
  "src/tile.cc"
  "src/world_factory.cc"
  "src/world.cc"
)

add_library(world ${src})

if (unittest)
  set(src_test
    "test/mocks/creaturectrl_mock.h"
    "test/mocks/itemfactory_mock.h"
    "test/position_test.cc"
    "test/creature_test.cc"
    "test/item_test.cc"
    "test/world_test.cc"
    "test/tile_test.cc"
  )

  add_executable(world_test ${src_test})
  target_link_libraries(world_test
    gtest_main
    gmock_main
    world
    utils_test_stubs
  )
endif()
