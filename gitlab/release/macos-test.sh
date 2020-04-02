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

ROOT_DIR=$PWD

#### Extract release package ####

OPENVKL_PKG_BASE=openvkl-0.9.0.x86_64.macos
unzip ${OPENVKL_PKG_BASE}.zip

#### Build tutorial against release package ####

export openvkl_DIR="${ROOT_DIR}/${OPENVKL_PKG_BASE}"

mkdir build_from_release
cd build_from_release

cmake --version

cmake ../examples/from_openvkl_install

cmake --build .

#### Run tutorial to verify functionality ###

export DYLD_LIBRARY_PATH=${openvkl_DIR}/lib:${DYLD_LIBRARY_PATH}

./vklTutorial
