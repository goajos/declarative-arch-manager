#!/usr/bin/env bash
set -e # terminate on any failure

DAMGR="./bin/damgr"
NVIM_DIR="/home/testuser/.config/nvim"
VIMRC="/etc/vimrc"

echo "---------------------"
echo "Testing merge command"
echo "---------------------"

if [ -d $NVIM_DIR ]; then
  echo "$NVIM_DIR already exists"
  exit 1
fi

if [ -f $VIMRC ]; then
  echo "$VIMRC already exists"
fi

yes | $DAMGR merge

if [ ! -d $NVIM_DIR ]; then
  echo "Testing merge failed to link nvim dotfiles"
  exit 1
fi

if [ ! -f $VIMRC ]; then
  echo "Testing merge failed to create vimrc with post hook"
  exit 1
fi

# second run to test comparison
yes | $DAMGR merge

echo "--------------------"
echo "Testing merge passed"
echo "--------------------"
