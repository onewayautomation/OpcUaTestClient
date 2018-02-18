@echo off
SET SCR_PATH=%~dp0
for %%i in ("%~dp0.") do SET "SCR_PATH=%%~fi"

pushd %SCR_PATH%\..
IF EXIST 1WaOpcUaSdk GOTO END_SDK_CLONE
git clone --recursive --depth 3 https://github.com/onewayautomation/1WaOpcUaSdk.git 1WaOpcUaSdk
:END_SDK_CLONE
popd

REM =============== Installing boost ... ======================
call %SCR_PATH%\build-boost.cmd
REM =============== boost has been installed! =================

REM =============== Installing botan ... ======================
call %SCR_PATH%\build-botan.cmd
REM =============== botan has been installed! =================
