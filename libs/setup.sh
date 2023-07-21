#!/bin/sh

set -xe

ZLIB_VERSION='v1.2.13'
ZLIB_GIT_REPO='https://github.com/madler/zlib'

if [ -d 'zlib' ]; then
    read -p 'zlib seems to be already installed, would you like to redownload it? Y/[N] ' -r REPLY
    if [ "$REPLY" = 'Y' -o "$REPLY" = 'y' ]; then
        echo 'Redownloading zlib...'
        rm -rf ./zlib
        git clone --depth 1 --branch "$ZLIB_VERSION" "$ZLIB_GIT_REPO"
        echo 'zlib downloaded!'
    else
        echo 'zlib will not be redownloaded.'
    fi
else
    echo 'Downloading zlib...'
    git clone --depth 1 --branch "$ZLIB_VERSION" "$ZLIB_GIT_REPO"
    echo 'zlib downloaded!'
fi
