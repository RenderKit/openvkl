#!/bin/bash
## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

mkdir build
cd build

# NOTE(jda) - Some Linux OSs need to have TBB on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=`pwd`/install/lib:${LD_LIBRARY_PATH}

cmake --version

cmake \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DBUILD_OPENVKL_BENCHMARKS=ON \
  "$@" ../superbuild

cmake --build .
