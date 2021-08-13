@echo off
rem Copyright 2019-2021 Intel Corporation
rem SPDX-License-Identifier: Apache-2.0

setlocal

echo Running tests

set PATH=%PATH%;.\build\install\bin

call .\build\install\bin\vklTutorial.exe
call .\build\install\bin\vklTutorialISPC.exe
call .\build\install\bin\vklTests.exe

:abort
endlocal
:end

rem propagate any error to calling PowerShell script:
exit

