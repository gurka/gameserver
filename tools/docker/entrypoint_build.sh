#!/bin/bash
set -xe
if [ ! -d "/gameserver" ]
then
  echo "gameserver root directory must be attached to /gameserver"
  echo "--attach .:/gameserver"
  exit 1
fi

source /build/emsdk/emsdk_env.sh

mkdir -p /gameserver/docker_build
cd /gameserver/docker_build
CC=clang-9 CXX=clang++-9 cmake ../gameserver -DGAMESERVER=ON -DCMAKE_BUILD_TYPE=debug
make
chown -R $USER_ID:$GROUP_ID /gameserver/docker_build

mkdir -p /gameserver/docker_build_wsclient
cd /gameserver/docker_build_wsclient
CC=emcc CXX=em++ cmake ../gameserver -DWSCLIENT=ON -DCMAKE_BUILD_TYPE=debug
make
chown -R $USER_ID:$GROUP_ID /gameserver/docker_build_wsclient
