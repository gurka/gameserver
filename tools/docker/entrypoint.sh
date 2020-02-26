#!/bin/bash
set -xe
if [ ! -d "/gameserver" ]
then
  echo "gameserver root directory must be attached to /gameserver"
  echo "--attach .:/gameserver"
  exit 1
fi

usermod -u $USER_ID user
groupmod -g $GROUP_ID user
export PATH="/build/emsdk:/build/emsdk/node/12.9.1_64bit/bin:/build/emsdk/upstream/emscripten:$PATH"
export USER="user"
export LOGNAME="user"
export HOME="/gameserver"
cd
su --preserve-environment user
