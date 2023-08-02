#!/bin/sh

set -xe

if [ ! -d 'out' ]; then
    mkdir out
fi

DIST=$(echo "$1" | tr '[:lower:]' '[:upper:]')
DFLAGS=''

PNG_SRC=$(find src/png -type f -name *.cpp)

case "$DIST" in
    'DBG')
        echo 'Debug build selected.'
        DFLAGS="$DFLAGS-Og -g -DPNG_DEBUG "
    ;;
    'OPT')
        echo 'Optimized build selected.'
        DFLAGS="$DFLAGS-O3 "
    ;;
    *)
        echo 'Normal build selected.'
    ;;
esac

CFLAGS="-std=c++2a -Wall -Wextra -Isrc -Ilibs/include -Llibs -lz -lpthread $DFLAGS"

g++ -o out/main src/main.cpp $PNG_SRC $CFLAGS
