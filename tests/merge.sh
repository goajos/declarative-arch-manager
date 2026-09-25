#!/usr/bin/env bash

DAMGR="./bin/damgr"
echo "Testing merge command"
$DAMGR merge
test -d /home/testuser/.config/nvim || (echo "Testing merge failed to link nvim dotfiles" && exit 1)
test -O /etc/vimrc || (echo "Testing merge failed to create vimrc with post hook" && exit 1)

echo "Testing merge passed"
