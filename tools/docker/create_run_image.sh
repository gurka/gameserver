#!/bin/bash
cd "$(git rev-parse --show-toplevel)"
docker build -t gameserver_run -f tools/docker/Dockerfile.run .
