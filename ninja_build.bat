:: Copyright 2020 Benbuck Nason

setlocal enableextensions

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

set BUILDDIR=build\ninja\%CONFIG%
if not exist %BUILDDIR% mkdir %BUILDDIR%
pushd %BUILDDIR%

cmake ..\..\.. -G Ninja -DCMAKE_BUILD_TYPE=%CONFIG%
cmake --build .

if "%CONFIG%"=="Release" cpack

popd
