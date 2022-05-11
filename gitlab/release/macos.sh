#!/bin/bash
## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

#### Helper functions ####

umask=`umask`
function onexit {
  umask $umask
}
trap onexit EXIT
umask 002

#### Set variables for script ####

ROOT_DIR=$PWD

DEP_BUILD_DIR=$ROOT_DIR/build_deps
DEP_INSTALL_DIR=$ROOT_DIR/install_deps

OPENVKL_PKG_BASE=openvkl-${OPENVKL_RELEASE_PACKAGE_VERSION}.x86_64.macos
OPENVKL_BUILD_DIR=$ROOT_DIR/build_release
OPENVKL_INSTALL_DIR=$ROOT_DIR/install_release/$OPENVKL_PKG_BASE

THREADS=`sysctl -n hw.logicalcpu`

# to make sure we do not include nor link against wrong TBB
unset CPATH
unset LIBRARY_PATH
unset DYLD_LIBRARY_PATH

#### Cleanup any existing directories ####

rm -rf $DEP_INSTALL_DIR
rm -rf $DEP_BUILD_DIR
rm -rf $OPENVKL_BUILD_DIR
rm -rf $OPENVKL_INSTALL_DIR

#### Build dependencies ####

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

cmake --version

cmake \
  "$@" \
  -D BUILD_DEPENDENCIES_ONLY=ON \
  -D CMAKE_INSTALL_PREFIX=$DEP_INSTALL_DIR \
  -D CMAKE_INSTALL_LIBDIR=lib \
  ../superbuild

cmake --build .

cd $ROOT_DIR

#### Build Open VKL ####

mkdir -p $OPENVKL_BUILD_DIR
cd $OPENVKL_BUILD_DIR

# Setup environment variables for dependencies
export rkcommon_DIR=$DEP_INSTALL_DIR
export embree_DIR=$DEP_INSTALL_DIR
export glfw3_DIR=$DEP_INSTALL_DIR

export OPENVKL_EXTRA_OPENVDB_OPTIONS="-DCMAKE_NO_SYSTEM_FROM_IMPORTED=ON"

# set release settings
cmake -L \
  -D CMAKE_INSTALL_PREFIX=$OPENVKL_INSTALL_DIR \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=doc \
  -D CMAKE_INSTALL_BINDIR=bin \
  -D RKCOMMON_TBB_ROOT=$DEP_INSTALL_DIR \
  -D ISPC_EXECUTABLE=$DEP_INSTALL_DIR/bin/ispc \
  -D BUILD_BENCHMARKS=ON \
  -D OpenVDB_ROOT=$DEP_INSTALL_DIR $OPENVKL_EXTRA_OPENVDB_OPTIONS \
  ..

# build
make -j $THREADS install

# copy dependent libs into the install
INSTALL_LIB_DIR=$OPENVKL_INSTALL_DIR/lib

cp -P $DEP_INSTALL_DIR/lib/lib*.dylib* $INSTALL_LIB_DIR

# zip up the results
cd $OPENVKL_INSTALL_DIR/..
zip -ry $OPENVKL_PKG_BASE.zip $OPENVKL_PKG_BASE

# sign
$ROOT_DIR/gitlab/release/sign.sh $OPENVKL_PKG_BASE.zip

mv *.zip $ROOT_DIR
