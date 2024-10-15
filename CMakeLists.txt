# Copyright 2020 Benbuck Nason
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.28)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    cmake_policy(SET CMP0117 NEW) # /GR (RTTI) is not added to CMAKE_CXX_FLAGS by default
endif()

project(Finestray
    VERSION 0.1
)

set(COMPILE_CONFIG_OPTIONS
    $<$<CXX_COMPILER_ID:MSVC>:
        /D_HAS_EXCEPTIONS=0 # disable exceptions
        /GR- # disable run-time type information (RTTI)
        /GS # enable security checks
        /MP # build with multiple processes
        /W4 # maximum warning levels
        /WX # treat warnings as errors

        # Debug compile options
        $<$<CONFIG:Debug>:
            /Od # don't optimize
            /RTC1 # enable all run-time checks
            /Zi # enable debug info in pdb format
        >

        # Release compile options
        $<$<CONFIG:Release>:
            /O2 # enable maximum speed optimizations
            /Ot # favor fast code
            /GL # enable whole program optimization
        >
    >

    $<$<CXX_COMPILER_ID:Clang>:
        -fno-exceptions # disable exceptions
        -fno-rtti # disable run-time type information (RTTI)
        -Wall # enable all warnings
        -Wextra # enable extra warnings
        -Werror # treat warnings as errors

        # Debug compile options
        $<$<CONFIG:Debug>:
            -O0 # don't optimize
            -g # enable debug info
        >

        # Release compile options
        $<$<CONFIG:Release>:
            -O3 # enable maximum speed optimizations
            -flto # enable link time optimization
        >
    >
)

set(LINK_CONFIG_OPTIONS
    $<$<CXX_COMPILER_ID:MSVC>:
        # Release link options
        $<$<CONFIG:Release>:
            /LTCG # link time code generation
        >
    >

    $<$<CXX_COMPILER_ID:Clang>:
        # Release link options
        $<$<CONFIG:Release>:
            -flto # link time optimization
        >
    >
)

include(argh.cmake)
include(cJSON.cmake)

add_executable(Finestray WIN32
    src/AppName.h
    src/BitmapWrapper.h
    src/CommandLine.cpp
    src/CommandLine.h
    src/DebugPrint.cpp
    src/DebugPrint.h
    src/File.cpp
    src/File.h
    src/Finestray.cpp
    src/Finestray.h
    src/Finestray.ico
    src/Finestray.rc
    src/Hotkey.cpp
    src/Hotkey.h
    src/MenuWrapper.h
    src/Resource.h
    src/Settings.cpp
    src/Settings.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/StringUtility.cpp
    src/StringUtility.h
    src/TrayIcon.cpp
    src/TrayIcon.h
    src/TrayWindow.cpp
    src/TrayWindow.h
    src/WindowList.cpp
    src/WindowList.h
)

target_compile_options(Finestray
    PRIVATE
        ${COMPILE_CONFIG_OPTIONS}
)

target_compile_definitions(Finestray
    PRIVATE
        UNICODE # Windows headers use wide character by default
)

target_link_options(Finestray
    PRIVATE
        ${LINK_CONFIG_OPTIONS}
)

target_link_libraries(Finestray
    PRIVATE
        argh
        cJSON

        Comctl32.lib
        Shlwapi.lib
)

install(
    TARGETS Finestray
    RUNTIME
    DESTINATION .
)

set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Finestray")
set(CPACK_PACKAGE_VENDOR "Benbuck Nason")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "Finestray" "Finestray" )
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMSTARTUP\\\\Finestray.lnk' '$INSTDIR\\\\Finestray.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMSTARTUP\\\\Finestray.lnk'")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "Exec '\\\"$INSTDIR\\\\Finestray.exe\\\"'")
set(CPACK_NSIS_INSTALLED_ICON_NAME "Finestray.exe")
set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_SOURCE_DIR}\\\\src\\\\images\\\\installer_header.bmp")
set(CPACK_NSIS_HELP_LINK "https://github.com/benbuck/finestray/")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}\\\\src\\\\images\\\\installer_welcome.bmp")
set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CMAKE_SOURCE_DIR}\\\\src\\\\images\\\\installer_welcome.bmp")
include(CPack)

if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Finestray)
endif()
