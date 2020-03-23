#!/bin/bash
## ======================================================================== ##
## Copyright 2020 Intel Corporation                                         ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

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

OPENVKL_PKG_BASE=openvkl-0.9.0.x86_64.macos
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
export OSPCOMMON_TBB_ROOT=$DEP_INSTALL_DIR
export ospcommon_DIR=$DEP_INSTALL_DIR
export embree_DIR=$DEP_INSTALL_DIR
export glfw3_DIR=$DEP_INSTALL_DIR

export OPENVKL_EXTRA_OPENVDB_OPTIONS="-DOpenVDB_ABI=7 -DCMAKE_NO_SYSTEM_FROM_IMPORTED=ON"

# set release settings
cmake -L \
  -D CMAKE_INSTALL_PREFIX=$OPENVKL_INSTALL_DIR \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=doc \
  -D CMAKE_INSTALL_BINDIR=bin \
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
mv *.zip $ROOT_DIR
