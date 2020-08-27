:: Copyright 2020 Benbuck Nason

setlocal enableextensions

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

set BUILDDIR=build\vstudio
if not exist %BUILDDIR% mkdir %BUILDDIR%
pushd %BUILDDIR%

cmake ..\..
cmake --build . --config %CONFIG%%

if "%CONFIG%"=="Release" cpack

popd
