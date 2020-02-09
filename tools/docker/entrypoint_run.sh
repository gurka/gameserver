#!/bin/bash

if [ "$1" == "login" ]
then
  exec ./loginserver
elif [ "$1" == "world" ]
then
  exec ./worldserver
else
  echo "Usage: $0 login|world"
  exit 1
fi
