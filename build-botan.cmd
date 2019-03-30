rem @echo off
SET vc_bat_name1="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"

rem SET ARCHITECTURE=amd64
SET ARCHITECTURE=x64

IF EXIST %vc_bat_name1% GOTO VC1
echo Failed to find Visual Studio 2017 batch file to setup environment.
echo Please check in the Visual Studio Installer that option "Desktop development with C++" is selected for Visual Studio 2017 Community Edition.
echo If you have other edition of Visual Studio 2017, please modify this file accordingly.
exit
:VC1
call %vc_bat_name1% %ARCHITECTURE%
GOTO continue

:continue

%~d0

SET SCRIPT_PATH=%~dp0
for %%i in ("%~dp0.") do SET "SCRIPT_PATH=%%~fi"
SET REPO_BASE_FOLDER=%SCRIPT_PATH%\..
SET TAG_BOTAN=2.9.0
set PATH_BOTAN=botan

PUSHD %REPO_BASE_FOLDER%

IF EXIST %PATH_BOTAN% GOTO BUILD_BOTAN
echo Cloning Botan ...
git clone --recursive --branch %TAG_BOTAN% --depth 1 https://github.com/randombit/botan.git %PATH_BOTAN%

:BUILD_BOTAN
PUSHD %PATH_BOTAN%
IF EXIST %REPO_BASE_FOLDER%\%PATH_BOTAN%\install\debug\botan.lib GOTO BUILD_BOTAN_RELEASE
python configure.py --debug-mode --prefix=%REPO_BASE_FOLDER%\%PATH_BOTAN%\install\debug
nmake BUILD=debug
nmake install
:BUILD_BOTAN_RELEASE
IF EXIST %REPO_BASE_FOLDER%\%PATH_BOTAN%\install\release\botan.lib GOTO END_BUILD_BOTAN
python configure.py --prefix=%REPO_BASE_FOLDER%\%PATH_BOTAN%\install\release
nmake BUILD=release
nmake install

:END_BUILD_BOTAN
POPD

POPD
