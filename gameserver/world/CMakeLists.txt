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
