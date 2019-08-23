#! /bin/bash

INSTALL_DIR=$HOME/local
PY_VERSION=python3.5

export LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib:$INSTALL_DIR/lib/x86_64-linux-gnu
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib:$INSTALL_DIR/lib:$INSTALL_DIR/lib/x86_64-linux-gnu
export PYTHONPATH=${INSTALL_DIR}/lib/${PY_VERSION}/site-packages:${INSTALL_DIR}/lib/${PY_VERSION}/dist-packages

cd build
../test/testnet.py
../test/testnet-edge.py
