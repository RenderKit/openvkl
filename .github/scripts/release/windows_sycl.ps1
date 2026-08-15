## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# abort on any error
$ErrorActionPreference = 'Stop'
function Assert-Success { if ($LASTEXITCODE) { exit $LASTEXITCODE } }

# Copy-Item silently ignores wildcards matching nothing
function Copy-Required($Path, $Destination) {
  $files = @(Get-ChildItem $Path)
  if (!$files) { throw "nothing matches $Path" }
  Copy-Item $files $Destination
}

#### Set variables for script ####

$ROOT_DIR = pwd

$DEP_BUILD_DIR = "$ROOT_DIR\build_deps"
$DEP_INSTALL_DIR = "$ROOT_DIR\install_deps"

$OPENVKL_PKG_BASE = "openvkl-$OPENVKL_RELEASE_PACKAGE_VERSION.sycl.x86_64.windows"
$OPENVKL_BUILD_DIR = "$ROOT_DIR/build_release"
$OPENVKL_INSTALL_DIR = "$ROOT_DIR/install_release/$OPENVKL_PKG_BASE"

$THREADS = $env:NUMBER_OF_PROCESSORS

#### Cleanup any existing directories ####

rm -Recurse -Force -ErrorAction SilentlyContinue `
  $DEP_INSTALL_DIR, $DEP_BUILD_DIR, $OPENVKL_BUILD_DIR, $OPENVKL_INSTALL_DIR

#### Build dependencies ####

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

cmake --version

cmake `
  $args `
  -G Ninja `
  -D BUILD_DEPENDENCIES_ONLY=ON `
  -D BUILD_OPENVKL_BENCHMARKS=ON `
  -D CMAKE_INSTALL_PREFIX=$DEP_INSTALL_DIR `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D CMAKE_CXX_COMPILER=clang-cl -D CMAKE_C_COMPILER=clang-cl `
  ../superbuild
Assert-Success

cmake --build . --config Release --parallel $THREADS
Assert-Success

cd $ROOT_DIR

#### Build Open VKL ####

mkdir $OPENVKL_BUILD_DIR
cd $OPENVKL_BUILD_DIR

# set release settings
cmake -L `
  -G Ninja `
  -D CMAKE_PREFIX_PATH="$DEP_INSTALL_DIR" `
  -D CMAKE_INSTALL_PREFIX="$OPENVKL_INSTALL_DIR" `
  -D CMAKE_INSTALL_INCLUDEDIR=include `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D CMAKE_INSTALL_DOCDIR=doc `
  -D CMAKE_INSTALL_BINDIR=bin `
  -D CMAKE_BUILD_TYPE=Release `
  -D RKCOMMON_TBB_ROOT=$DEP_INSTALL_DIR `
  -D ISPC_EXECUTABLE=$DEP_INSTALL_DIR/bin/ispc.exe `
  -D BUILD_BENCHMARKS=ON `
  -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang `
  -D OPENVKL_ENABLE_DEVICE_GPU=ON `
  -D OpenVDB_ROOT=$DEP_INSTALL_DIR `
  -D CMAKE_NO_SYSTEM_FROM_IMPORTED=ON `
  ..
Assert-Success

# build and install
cmake --build . --config Release --parallel $THREADS --target install
Assert-Success

# copy dependent libs into the install
$INSTALL_BIN_DIR = "$OPENVKL_INSTALL_DIR/bin"
$INSTALL_LIB_DIR = "$OPENVKL_INSTALL_DIR/lib"

Copy-Required $DEP_INSTALL_DIR/bin/*.dll $INSTALL_BIN_DIR

# openvklConfig.cmake globs for the dependencies' import libs
Copy-Required $DEP_INSTALL_DIR/lib/rkcommon*.lib $INSTALL_LIB_DIR
Copy-Required $DEP_INSTALL_DIR/lib/embree*.lib $INSTALL_LIB_DIR
Copy-Required $DEP_INSTALL_DIR/lib/tbb*.lib $INSTALL_LIB_DIR

# the debug variants of TBB are not needed
rm $INSTALL_BIN_DIR/*_debug.dll
rm $INSTALL_LIB_DIR/*_debug.lib

# copy SYCL runtime dependencies
$SYCL_BIN_FILE = (get-command clang++).Path
$SYCL_BIN_DIR = Split-Path -Parent "$SYCL_BIN_FILE"

Copy-Required ${SYCL_BIN_DIR}/sycl?.dll $INSTALL_BIN_DIR
Copy-Required ${SYCL_BIN_DIR}/ur_loader.dll $INSTALL_BIN_DIR
Copy-Required ${SYCL_BIN_DIR}/ur_adapter_level_zero.dll $INSTALL_BIN_DIR
Copy-Required ${SYCL_BIN_DIR}/ur_adapter_level_zero_v?.dll $INSTALL_BIN_DIR
Copy-Required ${SYCL_BIN_DIR}/ur_win_proxy_loader.dll $INSTALL_BIN_DIR

# sign
;& $env:SIGN_FILE_WINDOWS -q -vv (Get-ChildItem $INSTALL_BIN_DIR\* | Select-Object -Expand FullName)

# zip up the results
$OPENVKL_PKG_BASE_ZIP = "$OPENVKL_PKG_BASE.zip"
cd $OPENVKL_INSTALL_DIR/..
Compress-Archive -Path $OPENVKL_PKG_BASE -DestinationPath $OPENVKL_PKG_BASE_ZIP
mv *.zip $ROOT_DIR

exit $LASTEXITCODE
