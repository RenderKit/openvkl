## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

$ROOT_DIR = pwd

#### Extract release package ####

$OPENVKL_PKG_BASE = "openvkl-$OPENVKL_RELEASE_PACKAGE_VERSION.x86_64.windows"
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

#### Run tutorial to verify functionality ####

$env:PATH += ";$env:openvkl_DIR\bin"

.\Release\vklTutorial.exe

#### Run binaries from release package to verify functionality ####

vklMinimal_06.exe

exit $LASTEXITCODE
