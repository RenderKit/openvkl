#!/bin/bash
## Copyright 2020-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

#### Helper functions ####

function check_symbols
{
  for sym in `nm $1 | grep $2_`
  do
    version=(`echo $sym | sed 's/.*@@\(.*\)$/\1/g' | grep -E -o "[0-9]+"`)
    if [ ${#version[@]} -ne 0 ]; then
      if [ ${#version[@]} -eq 1 ]; then version[1]=0; fi
      if [ ${#version[@]} -eq 2 ]; then version[2]=0; fi
      if [ ${version[0]} -gt $3 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
      if [ ${version[0]} -lt $3 ]; then continue; fi

      if [ ${version[1]} -gt $4 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
      if [ ${version[1]} -lt $4 ]; then continue; fi

      if [ ${version[2]} -gt $5 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
    fi
  done
}

function check_imf
{
for lib in "$@"
do
  if [ -n "`ldd $lib | fgrep libimf.so`" ]; then
    echo "Error: dependency to 'libimf.so' found"
    exit 3
  fi
done
}

#### Set variables for script ####

ROOT_DIR=$PWD

DEP_BUILD_DIR=$ROOT_DIR/build_deps
DEP_INSTALL_DIR=$ROOT_DIR/install_deps

OPENVKL_PKG_BASE=openvkl-${OPENVKL_RELEASE_PACKAGE_VERSION}.x86_64.linux
OPENVKL_BUILD_DIR=$ROOT_DIR/build_release
OPENVKL_INSTALL_DIR=$ROOT_DIR/install_release/$OPENVKL_PKG_BASE

THREADS=`nproc`

#### Cleanup any existing directories ####

rm -rf $DEP_INSTALL_DIR
rm -rf $DEP_BUILD_DIR
rm -rf $OPENVKL_BUILD_DIR
rm -rf $OPENVKL_INSTALL_DIR

#### Build dependencies ####

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

# NOTE(jda) - Some Linux OSs need to have lib/ on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=$DEP_INSTALL_DIR/lib:${LD_LIBRARY_PATH}

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

# verify libs
check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl.so GLIBC   2 17 0
check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl.so GLIBCXX 3 4 19
check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl.so CXXABI  1 3 7

check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver.so GLIBC   2 17 0
check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver.so GLIBCXX 3 4 19
check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver.so CXXABI  1 3 7

check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver_4.so GLIBC   2 17 0
check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver_4.so GLIBCXX 3 4 19
check_symbols $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver_4.so CXXABI  1 3 7

check_imf $OPENVKL_INSTALL_DIR/lib/libopenvkl.so
check_imf $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver.so
check_imf $OPENVKL_INSTALL_DIR/lib/libopenvkl_module_ispc_driver_4.so

# copy dependent libs into the install
INSTALL_LIB_DIR=$OPENVKL_INSTALL_DIR/lib

cp -P $DEP_INSTALL_DIR/lib/lib*.so* $INSTALL_LIB_DIR

# tar up the results
cd $OPENVKL_INSTALL_DIR/..
tar -caf $OPENVKL_PKG_BASE.tar.gz $OPENVKL_PKG_BASE
mv *.tar.gz $ROOT_DIR
