#!/bin/bash -e

WEAVEDIR=`pwd`
WORKDIR=$HOME/prereq
INSTALL_DIR=$HOME/local

BUILDTYPE=release
export CC=gcc-8
export CXX=g++-8

export LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib

function clone-repo() {
    dir=$1
    url=$2

    if [ -d $dir ]; then
        echo "Repo ${url} already cloned."
        return 1
    else
        git clone $url $dir
        return 0
    fi
}

ssh-keyscan github.com >> ~/.ssh/known_hosts

cd $WORKDIR
clone-repo "bitstream" "https://github.com/kaimast/bitstream.git"
cd bitstream
git pull
echo "Building bitstream"
meson build -Dbuildtype=$BUILDTYPE --prefix=$INSTALL_DIR
cd build
ninja -v
ninja install -v

cd $WORKDIR
clone-repo "yael" "https://github.com/kaimast/yael.git"
cd yael
git pull
echo "Building yael"
git pull
meson build -Dbotan_dir=$INSTALL_DIR/include/botan-2 -Dbuildtype=$BUILDTYPE  --prefix=$INSTALL_DIR
cd build
ninja -v
ninja install -v

cd $WORKDIR
clone-repo "libdocument" "https://github.com/kaimast/libdocument.git"
cd libdocument
git pull
echo "Building libdocument"
meson build -Dbuildtype=$BUILDTYPE --prefix=$INSTALL_DIR
cd build
ninja -v
ninja install -v
