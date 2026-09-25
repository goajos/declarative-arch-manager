#!/usr/bin/env bash
set -e # terminate on any failure

DAMGR="./bin/damgr"
CONFIG_DIR="/home/testuser/.config/damgr"
STATE_DIR="/home/testuser/.local/state/damgr"

echo "--------------------"
echo "Testing init command"
echo "--------------------"

if [ -d $CONFIG_DIR ]; then
  echo "$CONFIG_DIR already exists"
  exit 1
fi

if [ -d $STATE_DIR ]; then
  echo "$STATE_DIR already exists"
  exit 1
fi

$DAMGR init

if [ ! -d $CONFIG_DIR ]; then
  echo "Testing init failed to create config dir"
  exit 1
fi

if [ ! -d $STATE_DIR ]; then
  echo "Testing init failed to create state dir"
  exit 1
fi

echo "-------------------"
echo "Testing init passed"
echo "-------------------"
