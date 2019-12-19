#!/bin/bash
## ======================================================================== ##
## Copyright 2019 Intel Corporation                                         ##
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

export ROOT_DIR=`pwd`

export openvkl_DIR="${ROOT_DIR}/build/install/lib/cmake"

export ospcommon_DIR="${ROOT_DIR}/build/install/lib/cmake"
export OSPCOMMON_TBB_ROOT="${ROOT_DIR}/build/install"

mkdir build_from_install
cd build_from_install

cmake --version

cmake \
  "$@" ../examples/from_openvkl_install

cmake --build .

./vklTutorial
