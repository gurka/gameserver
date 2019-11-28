#!/bin/bash
cd "$(git rev-parse --show-toplevel)/tools/docker"
docker build -t gameserver_build -f Dockerfile.build .
