#!/bin/bash -xe
## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

export ROOT_DIR=`pwd`
export DEP_INSTALL_DIR="${ROOT_DIR}/build/install"

export openvkl_DIR=${DEP_INSTALL_DIR}

mkdir build_from_install
cd build_from_install

cmake --version

cmake \
  "$@" ../examples/from_openvkl_install

cmake --build .

if [ -f "./vklTutorialCPU" ]; then
  ./vklTutorialCPU
fi

if [ -f "./vklTutorialGPU" ]; then
  ./vklTutorialGPU
fi
