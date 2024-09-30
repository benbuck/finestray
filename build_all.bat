:: Copyright 2020 Benbuck Nason

setlocal enabledelayedexpansion

pushd %~dp0

call ninja_build.bat Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

call ninja_build.bat Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

call vstudio_build.bat Debug
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

call vstudio_build.bat Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b %ERRORLEVEL%
)

popd
