#!/bin/sh

set -xe

if [ ! -d 'out' ]; then
    mkdir out
fi

DIST=$(echo "$1" | tr '[:lower:]' '[:upper:]')
DFLAGS=''

case "$DIST" in
    'DBG') DFLAGS="$DFLAGS -g -DPNG_DEBUG" ;;
esac

SRC=$(find src -type f -name *.cpp)
CFLAGS="-std=c++2a -Wall -Wextra -Isrc -Ilibs -lz -lpthread $DFLAGS"

g++ -o out/main $SRC $CFLAGS
