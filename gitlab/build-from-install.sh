#!/bin/bash
## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

export ROOT_DIR=`pwd`

export openvkl_DIR="${ROOT_DIR}/build/install/lib/cmake"

mkdir build_from_install
cd build_from_install

cmake --version

cmake \
  "$@" ../examples/from_openvkl_install

cmake --build .

./vklTutorial
