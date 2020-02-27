#!/bin/bash
set -xe

cd "$(git rev-parse --show-toplevel)"
docker build -t gameserver/build -f tools/docker/Dockerfile .
docker run --rm -it \
  -v "$(pwd)":"/gameserver" \
  -v "/var/run/docker.sock":"/var/run/docker.sock" \
  -e USER_ID=$(id -u) \
  -e GROUP_ID=$(id -g) \
  -e DOCKER_ID=$(getent group docker | awk -F: '{ print $3 }') \
  gameserver/build
