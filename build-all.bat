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

@echo off
setlocal enabledelayedexpansion

pushd %~dp0

echo.
echo ---------------------------------------------------------------
echo Building ninja-msvc-build.bat Debug
call ninja-msvc-build.bat Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Building ninja-msvc-build.bat Release
call ninja-msvc-build.bat Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Building ninja-clang-build.bat Debug
call ninja-clang-build.bat Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Building ninja-clang-build.bat Release
call ninja-clang-build.bat Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Building vstudio-msvc-build.bat Debug
call vstudio-msvc-build.bat Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Building vstudio-msvc-build.bat Release
call vstudio-msvc-build.bat Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Building vstudio-clang-build.bat Debug
call vstudio-clang-build.bat Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Building vstudio-clang-build.bat Release
call vstudio-clang-build.bat Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

popd
