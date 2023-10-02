@echo off
rem Copyright 2019 Intel Corporation
rem SPDX-License-Identifier: Apache-2.0

setlocal

echo Running tests

set PATH=%PATH%;.\build\install\bin

call .\build\install\bin\vklTutorialCPU.exe
call .\build\install\bin\vklTutorialISPC.exe
call .\build\install\bin\vklExamplesCPU -batch -printStats -spp 50 -framebufferSize 1024 1024
call .\build\install\bin\vklTestsCPU.exe

:abort
endlocal
:end

rem propagate any error to calling PowerShell script:
exit

