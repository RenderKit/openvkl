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

$ROOT_DIR = pwd

#### Extract release package ####

$OPENVKL_PKG_BASE = "openvkl-0.9.0.x86_64.windows"
Expand-Archive .\$OPENVKL_PKG_BASE.zip -DestinationPath $ROOT_DIR

#### Build tutorial against release package ####

$env:openvkl_DIR = "$ROOT_DIR\$OPENVKL_PKG_BASE"

mkdir build_from_release
cd build_from_release

cmake --version

cmake -L `
  -G $args[0] `
  -T $args[1] `
  ../examples/from_openvkl_install

cmake --build . --config Release --target ALL_BUILD -- /m /nologo

#### Run tutorial to verify functionality ###

$env:PATH += ";$env:openvkl_DIR\bin"

.\Release\vklTutorial.exe

exit $LASTEXITCODE
