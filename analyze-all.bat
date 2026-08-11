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
echo Running ninja-msvc-build.bat Analyze
call ninja-msvc-build.bat Analyze
if %ERRORLEVEL% NEQ 0 (
    echo Analysis failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Running ninja-clang-build.bat Analyze
call ninja-clang-build.bat Analyze
if %ERRORLEVEL% NEQ 0 (
    echo Analysis failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Running vstudio-msvc-build.bat Analyze
call vstudio-msvc-build.bat Analyze
if %ERRORLEVEL% NEQ 0 (
    echo Analysis failed
    exit /b %ERRORLEVEL%
)

echo.
echo ---------------------------------------------------------------
echo Running vstudio-clang-build.bat Analyze
call vstudio-clang-build.bat Analyze
if %ERRORLEVEL% NEQ 0 (
    echo Analysis failed
    exit /b %ERRORLEVEL%
)

popd
