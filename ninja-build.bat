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
set BUILD_DIR=build\ninja\%BUILD_CONFIG%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

if not defined VCINSTALLDIR (
    rem get latest visual studio install location
    rem for /f "tokens=* USEBACKQ" %%x in (`powershell -command "(Get-CimInstance MSFT_VSInstance | Select-Object -Last 1).InstallLocation"`) do set VSTUDIO_INSTALL_DIR=%%x
    rem winget install vswhere
    for /f "tokens=* USEBACKQ" %%x in (`vswhere -latest -property installationPath`) do set VSTUDIO_INSTALL_DIR=%%x

    rem set environment for visual studio command line builds
    call "!VSTUDIO_INSTALL_DIR!\VC\Auxiliary\Build\vcvars64.bat"
)

:: configure build with cmake
cmake ..\..\.. -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_CONFIG%

:: build
cmake --build .

:: for release builds, also make the package
if "%BUILD_CONFIG%"=="Release" (
    cmake --build . --config %BUILD_CONFIG% --target package
)

popd
