#!/bin/bash

DAMGR="./bin/damgr"
echo "Testing init command"
$DAMGR init
test -d ~/.config/damgr || (echo "Testing init failed to created config dir" && exit 1)
test -d ~/.local/state/damgr || (echo "Testing init failed to created state dir" && exit 1)

echo "Testing init passed"
