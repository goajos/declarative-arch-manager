#!/usr/bin/env bash
set -e # terminate on any failure

DAMGR="./bin/damgr"
NVIM_DIR="/home/testuser/.config/nvim"

yes | $DAMGR merge

if [ ! -d $NVIM_DIR ]; then
  echo "Testing merge failed to link nvim dotfiles"
  exit 1
fi

if ! command -v nvim &>/dev/null; then
  echo "Testing merge failed nvim not found"
  exit 1
fi

echo "--------------------"
echo "Testing merge passed"
echo "--------------------"
