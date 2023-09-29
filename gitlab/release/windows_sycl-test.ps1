## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

$ROOT_DIR = pwd

#### Extract release package ####

$OPENVKL_PKG_BASE = "openvkl-$OPENVKL_RELEASE_PACKAGE_VERSION.sycl.x86_64.windows"
Expand-Archive .\$OPENVKL_PKG_BASE.zip -DestinationPath $ROOT_DIR

#### Build tutorial against release package ####

$env:openvkl_DIR = "$ROOT_DIR\$OPENVKL_PKG_BASE"

mkdir build_from_release
cd build_from_release

cmake --version

cmake -L `
  -G Ninja `
  -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang `
  -D CMAKE_BUILD_TYPE=Release `
  ../examples/from_openvkl_install

cmake --build . --config Release --verbose

ls

echo "=== build done ==="

#### Run tutorial to verify functionality ####

$env:PATH += ";$env:openvkl_DIR\bin"

./vklTutorialGPU.exe

# Note, binaries from the release package are verified in another job.

exit $LASTEXITCODE
