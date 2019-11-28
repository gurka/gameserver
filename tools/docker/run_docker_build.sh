#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker run -it --volume "$(pwd)":/gameserver -u "$(id -u ${USER}):$(id -g ${USER})" gameserver_build
