#!/bin/sh

set -xe

if [ ! -d 'out' ]; then
    mkdir out
fi

SRC=$(find src -type f -name *.cpp)
CFLAGS='-g -std=c++2a -Wall -Wextra -Isrc -Ilibs -lz'

g++ -o out/main $SRC $CFLAGS
