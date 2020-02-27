#!/bin/bash
set -xe
if [ ! -d "/gameserver" ]
then
  echo "gameserver root directory must be attached to /gameserver"
  echo "--attach .:/gameserver"
  exit 1
fi
if [ -z $USER_ID ]
then
  echo "\$USER_ID must be set"
  exit 1
fi
if [ -z $GROUP_ID ]
then
  echo "\$GROUP_ID must be set"
  exit 1
fi

addgroup --gid $GROUP_ID user
adduser --uid $USER_ID --gid $GROUP_ID --home /gameserver/.docker_home --disabled-password --gecos ""  user
addgroup --gid $DOCKER_ID dockerhost
usermod -aG dockerhost user

if [ ! -f "/gameserver/.docker_home/.emscripten" ]
then
  su user -c '/build/emsdk/emsdk activate latest'
  su user -c 'echo "source /build/emsdk/emsdk_env.sh && cd /gameserver" >> /gameserver/.docker_home/.bashrc'
fi
su --login user
