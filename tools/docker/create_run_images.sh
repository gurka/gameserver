#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker build -t gameserver/login -f tools/docker/Dockerfile.login .
docker build -t gameserver/world -f tools/docker/Dockerfile.world .
docker build -t gameserver/wsclient -f tools/docker/Dockerfile.wsclient .
