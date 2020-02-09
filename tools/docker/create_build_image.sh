#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker build -t gameserver_build -f tools/docker/Dockerfile.build .
