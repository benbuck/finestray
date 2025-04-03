:: Copyright 2020 Benbuck Nason
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::     http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

setlocal enabledelayedexpansion

:: set up build configuration (Debug/Release)
set BUILD_CONFIG=%1
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Debug

:: set up build dir
set BUILD_DIR=build\vstudio-msvc
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

:: configure build with cmake
cmake ..\..

:: build
cmake --build . --config %BUILD_CONFIG%

:: for release builds, also make the package
if "%BUILD_CONFIG%"=="Release" (
    cmake --build . --config %BUILD_CONFIG% --target PACKAGE
)

popd
