#!/bin/bash
## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

mkdir build
cd build

cmake --version

cmake \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DBUILD_OPENVKL_BENCHMARKS=ON \
  "$@" ../superbuild

cmake --build .
