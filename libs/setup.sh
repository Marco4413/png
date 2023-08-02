#!/bin/sh

set -xe

ZLIB_VERSION='v1.2.13'
ZLIB_GIT_REPO='https://github.com/madler/zlib'

# FMT_VERSION='10.0.0'
# FMT_GIT_REPO='https://github.com/fmtlib/fmt'

download() {
    libName=$1
    libVer=$2
    libUrl=$3

    if [ -d "$libName" ]; then
        read -p "$libName seems to be already installed, would you like to redownload it? Y/[N] " -r REPLY
        if [ "$REPLY" = 'Y' -o "$REPLY" = 'y' ]; then
            echo "Redownloading $libName..."
            rm -rf "./$libName"
            git clone --depth 1 --branch "$libVer" "$libUrl"
            echo "$libName downloaded!"
        else
            echo "$libName will not be redownloaded."
        fi
    else
        echo "Downloading $libName..."
        git clone --depth 1 --branch "$libVer" "$libUrl"
        echo "$libName downloaded!"
    fi
}

build_cmake() {
    libName=$1
    libStaticTarget=$2
    libStaticName=$3

    # Check if lib exists
    if [ ! -d "$libName" ]; then
        echo "$libName not downloaded."
        exit 1
    fi

    # Run cmake
    cd "$libName"
    cmake .
    if [ $? -ne 0 ]; then
        echo 'cmake failed to create make files'
        exit 1
    fi

    # Run configure if present
    if [ -e "configure" ]; then
        ./configure
        if [ $? -ne 0 ]; then
            echo 'configure failed'
            exit 1
        fi
    fi

    # Make static target
    make "$libStaticTarget"
    if [ $? -ne 0 ]; then
        echo "make failed to build $libName"
        exit 1
    fi

    # Copy lib into ..
    cp "$libStaticName" ..
    if [ $? -ne 0 ]; then
        echo "$libStaticName not found"
        exit 1
    fi

    if [ ! -d '../include' ]; then
        mkdir ../include
    fi

    # Copy either the contents of the include directory or all .h file in the current folder
    if [ -d 'include' ]; then
        cp -Ru include/* ../include/
    else
        if [ ! -d "../include/$libName" ]; then
            mkdir "../include/$libName"
        fi
        cp -u *.h "../include/$libName"
    fi

    cd ..
    echo "$libName built successfully!"
}

download zlib "$ZLIB_VERSION" "$ZLIB_GIT_REPO"
# download fmt "$FMT_VERSION" "$FMT_GIT_REPO"

rm -rf include
build_cmake zlib static libz.a
# build_cmake fmt fmt libfmt.a
