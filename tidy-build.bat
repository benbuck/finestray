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

:: set up build dir
set BUILD_DIR=%1
if "%BUILD_DIR%"=="" set BUILD_DIR=build/vstudio-msvc/Release

pushd %~dp0

:: set up visual studio environment
if not defined VCINSTALLDIR (
    for /f "tokens=* USEBACKQ" %%x in (`vswhere -latest -property installationPath`) do set VSTUDIO_INSTALL_DIR=%%x
    call "!VSTUDIO_INSTALL_DIR!\VC\Auxiliary\Build\vcvars64.bat"
)
set CLANG_TIDY_BIN=!VSTUDIO_INSTALL_DIR!\VC\Tools\Llvm\x64\bin\clang-tidy.exe
set RUN_CLANG_TIDY=!VSTUDIO_INSTALL_DIR!\VC\Tools\Llvm\x64\bin\run-clang-tidy
set CLANG_TIDY_OPTS=-j %NUMBER_OF_PROCESSORS% -clang-tidy-binary "!CLANG_TIDY_BIN!" -source-filter ".*\.cpp$" -quiet -format

echo python "!RUN_CLANG_TIDY!" -p %BUILD_DIR% %CLANG_TIDY_OPTS%
python "!RUN_CLANG_TIDY!" -p %BUILD_DIR% %CLANG_TIDY_OPTS%
if %ERRORLEVEL% NEQ 0 (
    echo Tidy failed
    exit /b %ERRORLEVEL%
)
