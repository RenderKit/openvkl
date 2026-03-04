## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

$ROOT_DIR = pwd

#### Extract release package ####

$OPENVKL_PKG_BASE = "openvkl-$OPENVKL_RELEASE_PACKAGE_VERSION.sycl.x86_64.windows"
Expand-Archive .\$OPENVKL_PKG_BASE.zip -DestinationPath $ROOT_DIR

#### Run binaries from release package to verify functionality ####

$env:openvkl_DIR = "$ROOT_DIR\$OPENVKL_PKG_BASE"
$env:PATH += ";$env:openvkl_DIR\bin"

vklTutorialGPU.exe

vklMinimal_06.exe

exit $LASTEXITCODE
