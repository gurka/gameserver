#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker run --rm -it --volume "$(pwd)":/gameserver -u "$(id -u ${USER}):$(id -g ${USER})" gameserver/build
