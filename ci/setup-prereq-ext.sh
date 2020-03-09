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

mkdir -p ${WORKDIR}
mkdir -p ${INSTALL_DIR}/include
mkdir -p ${INSTALL_DIR}/lib
mkdir -p ${INSTALL_DIR}/bin

cd $WORKDIR

git clone https://github.com/randombit/botan.git
pushd botan
    git pull
    echo "Building botan"
    git checkout release-2
    python ./configure.py --with-openssl --prefix=$INSTALL_DIR
    make -j10
    make install
popd
