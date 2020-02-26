#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker build -t gameserver/build .
docker run --rm -it \
  --volume "$(pwd)":/gameserver \
  -e USER_ID=$(id -u $USER) \
  -e GROUP_ID=$(id -g $USER) \
  -e EM_CACHE="/gameserver/.emscripten_cache" \
  gameserver/build
