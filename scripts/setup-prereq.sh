#! /bin/env bash

INSTALL_DIR=${INSTALL_DIR:=$HOME/local}
echo "Using installation directory \"${INSTALL_DIR}\""

PY_VERSION=`python3 --version | cut -d' ' -f2 | cut -d. -f1,2`
BUILDTYPE=release

export LIBRARY_PATH=$INSTALL_DIR/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$INSTALL_DIR/lib:$LD_LIBRARY_PATH
export PATH=$INSTALL_DIR/bin:$PATH

pushd deps
dependencies=("bitstream" "yael")
for name in "${dependencies[@]}"; do
    echo "Building ${name}"
    pushd $name 
    meson setup build -Dbuildtype=$BUILDTYPE  --prefix=$INSTALL_DIR
    pushd build
    ninja install -v
    popd
    popd
done
