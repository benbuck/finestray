:: Copyright 2020 Benbuck Nason

setlocal enabledelayedexpansion

:: set up build configuration (Debug/Release)
set BUILD_CONFIG=%1
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Debug

:: set up build dir
set BUILD_DIR=build\vstudio
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

:: configure build with cmake
cmake ..\..

:: build
cmake --build . --config %BUILD_CONFIG%%

:: for release builds, also make the package
if "%BUILD_CONFIG%"=="Release" cpack

popd
