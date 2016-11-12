cmake_minimum_required(VERSION 3.0)

project(gameengine)

set(src
  "src/gameengine.cc"
  "src/player.cc"
  "src/player.h"
  "export/gameengine.h"
  "export/playerctrl.h"
)

add_library(gameengine ${src})
