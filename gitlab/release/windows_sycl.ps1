## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

#### Set variables for script ####

$ROOT_DIR = pwd

$DEP_BUILD_DIR = "$ROOT_DIR\build_deps"
$DEP_INSTALL_DIR = "$ROOT_DIR\install_deps"

$OPENVKL_PKG_BASE = "openvkl-$OPENVKL_RELEASE_PACKAGE_VERSION.sycl.x86_64.windows"
$OPENVKL_BUILD_DIR = "$ROOT_DIR/build_release"
$OPENVKL_INSTALL_DIR = "$ROOT_DIR/install_release/$OPENVKL_PKG_BASE"

## Build dependencies ##

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

cmake --version

cmake -L `
  -G Ninja `
  -D BUILD_DEPENDENCIES_ONLY=ON `
  -D CMAKE_INSTALL_PREFIX=$DEP_INSTALL_DIR `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D CMAKE_CXX_COMPILER=clang-cl -D CMAKE_C_COMPILER=clang-cl `
  -D BUILD_OPENVDB=OFF `
  ../superbuild

cmake --build . --config Release --verbose

cd $ROOT_DIR

#### Build Open VKL ####

mkdir $OPENVKL_BUILD_DIR
cd $OPENVKL_BUILD_DIR

# Setup environment variables for dependencies
$env:rkcommon_DIR = $DEP_INSTALL_DIR
$env:embree_DIR = $DEP_INSTALL_DIR
$env:glfw3_DIR = $DEP_INSTALL_DIR

# set release settings
cmake -L `
  -G Ninja `
  -D CMAKE_PREFIX_PATH="$DEP_INSTALL_DIR\lib\cmake" `
  -D CMAKE_INSTALL_PREFIX="$OPENVKL_INSTALL_DIR" `
  -D CMAKE_INSTALL_INCLUDEDIR=include `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D CMAKE_INSTALL_DOCDIR=doc `
  -D CMAKE_INSTALL_BINDIR=bin `
  -D CMAKE_BUILD_TYPE=Release `
  -D RKCOMMON_TBB_ROOT=$DEP_INSTALL_DIR `
  -D ISPC_EXECUTABLE=$DEP_INSTALL_DIR/bin/ispc.exe `
  -D BUILD_BENCHMARKS=OFF `
  -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang `
  -D OPENVKL_ENABLE_DEVICE_GPU=ON `
  ..

# build
cmake --build . --config Release --verbose

# install
cmake --build . --config Release --target install

# copy dependent libs into the install
$INSTALL_BIN_DIR = "$OPENVKL_INSTALL_DIR/bin"
$INSTALL_LIB_DIR = "$OPENVKL_INSTALL_DIR/lib"

cp $DEP_INSTALL_DIR/bin/*.dll $INSTALL_BIN_DIR

cp $DEP_INSTALL_DIR/lib/rkcommon*.lib $INSTALL_LIB_DIR
cp $DEP_INSTALL_DIR/lib/embree*.lib $INSTALL_LIB_DIR
cp $DEP_INSTALL_DIR/lib/tbb*.lib $INSTALL_LIB_DIR

# copy SYCL runtime dependencies
$SYCL_BIN_FILE = where.exe clang++
$SYCL_BIN_DIR = Split-Path -Parent "$SYCL_BIN_FILE"

cp ${SYCL_BIN_DIR}/sycl7.dll $INSTALL_BIN_DIR
cp ${SYCL_BIN_DIR}/pi_win_proxy_loader.dll $INSTALL_BIN_DIR
cp ${SYCL_BIN_DIR}/pi_level_zero.dll $INSTALL_BIN_DIR

# sign
;& $env:SIGN_FILE_WINDOWS -q -vv (Get-ChildItem $INSTALL_BIN_DIR\* | Select-Object -Expand FullName)

# zip up the results
$OPENVKL_PKG_BASE_ZIP = "$OPENVKL_PKG_BASE.zip"
cd $OPENVKL_INSTALL_DIR/..
Compress-Archive -Path $OPENVKL_PKG_BASE -DestinationPath $OPENVKL_PKG_BASE_ZIP
mv *.zip $ROOT_DIR

exit $LASTEXITCODE
