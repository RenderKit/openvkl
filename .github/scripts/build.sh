#!/bin/bash
## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

MACOSX_DEPLOYMENT_TARGET="10.13"

mkdir build
cd build

cmake --version

cmake \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DBUILD_OPENVKL_BENCHMARKS=ON \
  "$@" ../superbuild

cmake --build .
