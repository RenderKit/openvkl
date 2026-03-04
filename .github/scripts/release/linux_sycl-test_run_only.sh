#!/bin/bash
## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# this job does not do any test builds, but rather verifies that the SYCL
# binaries can run without a SYCL compiler (driver is still needed).

# abort on any error
set -e

ROOT_DIR=$PWD

#### Extract release package ####

OPENVKL_PKG_BASE=openvkl-${OPENVKL_RELEASE_PACKAGE_VERSION}.sycl.x86_64.linux
tar -zxvf ${OPENVKL_PKG_BASE}.tar.gz

#### Run binaries from release package to verify functionality ####

export openvkl_DIR="${ROOT_DIR}/${OPENVKL_PKG_BASE}"
export LD_LIBRARY_PATH=${openvkl_DIR}/lib:${LD_LIBRARY_PATH}

${openvkl_DIR}/bin/vklTutorialGPU

${openvkl_DIR}/bin/vklMinimal_06
