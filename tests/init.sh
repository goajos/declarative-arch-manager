#!/usr/bin/env bash

DAMGR="./bin/damgr"
echo "Testing init command"
$DAMGR init
test -d ~/.config/damgr || (echo "Testing init failed to create config dir" && exit 1)
test -d ~/.local/state/damgr || (echo "Testing init failed to create state dir" && exit 1)

echo "Testing init passed"
