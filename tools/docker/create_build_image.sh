#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker build -t gameserver/build -f tools/docker/Dockerfile.build .
