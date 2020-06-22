#!/bin/bash
## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

export ROOT_DIR=`pwd`
export DEP_INSTALL_DIR="${ROOT_DIR}/build/install"

export openvkl_DIR=${DEP_INSTALL_DIR}
export rkcommon_DIR=${DEP_INSTALL_DIR}

mkdir build_from_install
cd build_from_install

cmake --version

cmake \
  -D RKCOMMON_TBB_ROOT=$DEP_INSTALL_DIR \
  "$@" ../examples/from_openvkl_install

cmake --build .

./vklTutorial
./vklTutorialLinkISPCDriver
