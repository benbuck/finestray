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

:: set up build configuration (Debug, Release, Analyze...)
set BUILD_CONFIG=%1
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Debug

:: set up build dir
set BUILD_DIR=%~dp0%build\ninja-msvc\!BUILD_CONFIG!
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
pushd %BUILD_DIR%

if not defined VCINSTALLDIR (
    rem get latest visual studio install location
    rem for /f "tokens=* USEBACKQ" %%x in (`powershell -command "(Get-CimInstance MSFT_VSInstance | Select-Object -Last 1).InstallLocation"`) do set VSTUDIO_INSTALL_DIR=%%x
    rem winget install vswhere
    for /f "tokens=* USEBACKQ" %%x in (`vswhere -latest -property installationPath`) do set VSTUDIO_INSTALL_DIR=%%x

    rem set environment for visual studio command line builds
    call "!VSTUDIO_INSTALL_DIR!\VC\Auxiliary\Build\vcvars64.bat"
    echo on
)

:: configure build with cmake
cmake ..\..\.. -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_CONFIG%

:: build
cmake --build . --parallel

:: for release builds, also make the installer
if "%BUILD_CONFIG%"=="Release" (
    set ISCC=ISCC.exe
    where !ISCC! >nul 2>nul
    if errorlevel 1 (
        if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
            set ISCC="%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
        ) else if exist "%ProgramData%\chocolatey\bin\ISCC.exe" (
            set ISCC="%ProgramData%\chocolatey\bin\ISCC.exe"
        ) else (
            echo ISCC.exe not found. Install Inno Setup 6 from https://jrsoftware.org/isinfo.php
            exit /b 1
        )
    )
    echo "!ISCC!" "%~dp0%installer\Finestray.iss" /DAppExePath="!BUILD_DIR!\Finestray.exe" /DIconPath="!BUILD_DIR!\Finestray.ico"
    "!ISCC!" "%~dp0%installer\Finestray.iss" /DAppExePath="!BUILD_DIR!\Finestray.exe" /DIconPath="!BUILD_DIR!\Finestray.ico"
)

popd
