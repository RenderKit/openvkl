## Copyright 2020-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

#### Set variables for script ####

$ROOT_DIR = pwd

$DEP_BUILD_DIR = "$ROOT_DIR\build_deps"
$DEP_INSTALL_DIR = "$ROOT_DIR\install_deps"

$OPENVKL_PKG_BASE = "openvkl-$OPENVKL_RELEASE_PACKAGE_VERSION.x86_64.windows"
$OPENVKL_BUILD_DIR = "$ROOT_DIR/build_release"
$OPENVKL_INSTALL_DIR = "$ROOT_DIR/install_release/$OPENVKL_PKG_BASE"

## Build dependencies ##

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

cmake --version

cmake -L `
  -G $args[0] `
  -T $args[1] `
  -D BUILD_DEPENDENCIES_ONLY=ON `
  -D CMAKE_INSTALL_PREFIX=$DEP_INSTALL_DIR `
  -D CMAKE_INSTALL_LIBDIR=lib `
  ../superbuild

cmake --build . --config Release --target ALL_BUILD -- /m /nologo

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
  -G $args[0] `
  -T $args[1] `
  -D CMAKE_PREFIX_PATH="$DEP_INSTALL_DIR\lib\cmake" `
  -D CMAKE_INSTALL_PREFIX="$OPENVKL_INSTALL_DIR" `
  -D CMAKE_INSTALL_INCLUDEDIR=include `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D CMAKE_INSTALL_DOCDIR=doc `
  -D CMAKE_INSTALL_BINDIR=bin `
  -D RKCOMMON_TBB_ROOT=$DEP_INSTALL_DIR `
  -D ISPC_EXECUTABLE=$DEP_INSTALL_DIR/bin/ispc.exe `
  -D BUILD_BENCHMARKS=OFF `
  ..

# build
cmake --build . --config Release --target ALL_BUILD -- /m /nologo

# install
cmake --build . --config Release --target install -- /m /nologo

# copy dependent libs into the install
$INSTALL_BIN_DIR = "$OPENVKL_INSTALL_DIR/bin"

cp $DEP_INSTALL_DIR/bin/*.dll $INSTALL_BIN_DIR

# zip up the results
$OPENVKL_PKG_BASE_ZIP = "$OPENVKL_PKG_BASE.zip"
cd $OPENVKL_INSTALL_DIR/..
Compress-Archive -Path $OPENVKL_PKG_BASE -DestinationPath $OPENVKL_PKG_BASE_ZIP
mv *.zip $ROOT_DIR

exit $LASTEXITCODE
