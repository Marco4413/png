#!/bin/sh

set -xe

mkdir -p out

DIST=$(echo "$1" | tr '[:lower:]' '[:upper:]')
DFLAGS=''

case "$DIST" in
    'DBG')
        echo 'Debug build selected.'
        DFLAGS="$DFLAGS-Og -g -DPNG_DEBUG "
    ;;
    'OPT')
        echo 'Optimized build selected.'
        DFLAGS="$DFLAGS-O3 "
    ;;
    'STATIC')
        echo 'Static build selected.'
        DFLAGS="$DFLAGS-O3 "
    ;;
    *)
        echo 'Normal build selected.'
    ;;
esac

CFLAGS="-std=c++2a -Wall -Wextra -Iinclude -Ilibs/include -Llibs -lz -lpthread $DFLAGS"

if [ "$DIST" = 'STATIC' ]; then
    mkdir -p obj
    for file in $(find src -type f -name \*.cpp ! -name 'main.cpp')
    do
        g++ -o "obj/$(basename "$file" .cpp).o" -c "$file" $CFLAGS
        if [ $? -ne 0 ]; then
            echo "Failed to compile $file"
            exit 1
        fi
    done

    ar -rcs libpng.a obj/*
else
    SRC=$(find src -type f -name \*.cpp)
    g++ -o out/main $SRC $CFLAGS
fi
