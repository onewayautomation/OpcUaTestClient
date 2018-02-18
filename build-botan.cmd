call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

SET REPO_BASE_FOLDER=e:\WorkSpace
SET TAG_BOTAN=2.4.0
set PATH_BOTAN=botan

PUSHD %REPO_BASE_FOLDER%

IF EXIST %PATH_BOTAN% GOTO BUILD_BOTAN
echo Cloning Botan ...
git clone --recursive --branch %TAG_BOTAN% --depth 1 https://github.com/randombit/botan.git %PATH_BOTAN%

:BUILD_BOTAN
PUSHD %PATH_BOTAN%
IF EXIST Makefile GOTO MAKE_BOTAN
python configure.py --debug-mode --prefix=%REPO_BASE_FOLDER%\build\botan

:MAKE_BOTAN
nmake BUILD=debug
nmake install
POPD

POPD