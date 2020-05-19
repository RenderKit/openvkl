#!/bin/bash -x
## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e
KW_PATH=/NAS/tools/kw
KW_SERVER_PATH=$KW_PATH/server
KW_CLIENT_PATH=$KW_PATH/client
export KLOCWORK_LTOKEN=/tmp/ltoken
echo "$KW_SERVER_IP;$KW_SERVER_PORT;$KW_USER;$KW_LTOKEN" > $KLOCWORK_LTOKEN

mkdir build
cd build

# NOTE(jda) - Some Linux OSs need to have TBB on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=`pwd`/install/lib:${LD_LIBRARY_PATH}

cmake --version

cmake \
  -DBUILD_JOBS=`nproc` \
  -DBUILD_DEPENDENCIES_ONLY=ON \
  -DBUILD_GLFW=OFF \
  "$@" ../superbuild

cmake --build .

mkdir openvkl_build
cd openvkl_build

export RKCOMMON_TBB_ROOT=`pwd`/../install
export rkcommon_DIR=`pwd`/../install
export embree_DIR=`pwd`/../install

cmake \
 -DISPC_EXECUTABLE=`pwd`/../install/bin/ispc \
 -DBUILD_EXAMPLES=OFF \
  ../..

# build
$KW_CLIENT_PATH/bin/kwinject make -j `nproc`
$KW_SERVER_PATH/bin/kwbuildproject --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/kw_tables kwinject.out
$KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load --force --name build-$CI_JOB_ID $KW_PROJECT_NAME $CI_PROJECT_DIR/kw_tables
echo "build-$CI_JOB_ID" > $CI_PROJECT_DIR/kw_build_number

