@echo off
rem Copyright 2019-2020 Intel Corporation
rem SPDX-License-Identifier: Apache-2.0

setlocal

md build
cd build

cmake --version

cmake -L ^
-G "%~1" ^
-T "%~2" ^
-D CMAKE_INSTALL_LIBDIR=lib ^
-D BUILD_OPENVKL_BENCHMARKS=OFF ^
-D BUILD_OPENVKL_TESTING=ON ^
../superbuild

cmake --build . --verbose --config Release --target ALL_BUILD -- /m /nologo

:abort
endlocal
:end

rem propagate any error to calling PowerShell script:
exit
