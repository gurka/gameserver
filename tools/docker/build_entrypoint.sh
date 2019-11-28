#!/bin/bash
set -xe
if [ ! -d "/gameserver" ]
then
  echo "gameserver root directory must be attached to /gameserver"
  echo "--attach .:/gameserver"
  exit 1
fi

mkdir -p /gameserver/docker_build
cd /gameserver/docker_build
CC=clang-9 CXX=clang++-9 cmake ../gameserver -DGAMESERVER=ON -DCMAKE_BUILD_TYPE=debug
make
