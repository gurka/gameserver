#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker run --rm -it --volume "$(pwd)":/gameserver -e USER_ID=$(id -u $USER) -e GROUP_ID=$(id -g $USER) gameserver/build
