:: Copyright 2020 Benbuck Nason

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
    for /f "tokens=* USEBACKQ" %%x in (`powershell -command "(Get-CimInstance MSFT_VSInstance | Select-Object -Last 1).InstallLocation"`) do set VSTUDIO_INSTALL_DIR=%%x

    rem NOTE: alternative way to get visual studio install location
    rem winget install vswhere
    rem for /f "tokens=* USEBACKQ" %%x in (`vswhere -latest -property installationPath`) do set VSTUDIO_INSTALL_DIR=%%x

    rem set environment for visual studio command line builds
    call "!VSTUDIO_INSTALL_DIR!\VC\Auxiliary\Build\vcvars64.bat"
)

:: configure build with cmake
cmake ..\..\.. -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_CONFIG%

:: build
cmake --build .

:: for release builds, also make the package
:: FIX - this isn't working yet
:: if "%BUILD_CONFIG%"=="Release" cpack

popd
