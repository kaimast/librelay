#!/bin/bash -e

WEAVEDIR=`pwd`
WORKDIR=$HOME/prereq
INSTALL_DIR=$HOME/local

BUILDTYPE=release
export CC=gcc-8
export CXX=g++-8

export LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib

ssh-keyscan github.com >> ~/.ssh/known_hosts

cd $WORKDIR

git clone https://github.com/kaimast/bitstream.git
pushd bitstream
    git pull
    echo "Building bitstream"
    meson build -Dbuildtype=$BUILDTYPE --prefix=$INSTALL_DIR
    pushd build
        ninja -v
        ninja install -v
    popd
popd

git clone https://github.com/kaimast/yael.git
pushd yael
    echo "Building yael"
    git pull
    meson build -Dbotan_dir=$INSTALL_DIR/include/botan-2 -Dbuildtype=$BUILDTYPE  --prefix=$INSTALL_DIR
    pushd build
        ninja -v
        ninja install -v
    popd
popd

git clone https://github.com/kaimast/libdocument.git
pushd libdocument
    git pull
    echo "Building libdocument"
    meson build -Dbuildtype=$BUILDTYPE --prefix=$INSTALL_DIR
    pushd build
        ninja -v
        ninja install -v
    popd
popd
