# Copyright 2020 Benbuck Nason

cmake_minimum_required(VERSION 3.28)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    cmake_policy(SET CMP0117 NEW) # /GR (RTTI) is not added to CMAKE_CXX_FLAGS by default
endif()

project(MinTray
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

include(cJSON.cmake)

add_library(argh
    INTERFACE
        src/argh/argh.h
)

target_include_directories(argh
    INTERFACE
    src/argh
)

add_executable(MinTray WIN32
    src/CommandLine.cpp
    src/CommandLine.h
    src/DebugPrint.cpp
    src/DebugPrint.h
    src/File.cpp
    src/File.h
    src/Hotkey.cpp
    src/Hotkey.h
    src/MinTray.cpp
    src/MinTray.ico
    src/MinTray.rc
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

target_compile_options(MinTray
    PRIVATE
        ${COMPILE_CONFIG_OPTIONS}
)

target_compile_definitions(MinTray
    PRIVATE
        UNICODE # Windows headers use wide character by default
)

target_link_options(MinTray
    PRIVATE
        ${LINK_CONFIG_OPTIONS}
)

target_link_libraries(MinTray
    PRIVATE
        argh
        cJSON

        Comctl32.lib
        Shlwapi.lib
)

install(
    TARGETS MinTray
    RUNTIME DESTINATION .
    COMPONENT Runtime
)

set(CPACK_GENERATOR "NSIS64")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_INSTALLED_ICON_NAME "MinTray.exe")
set(CPACK_NSIS_HELP_LINK "https://github.com/benbuck/mintray/")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}\\\\src\\\\MinTray.ico")
# FIX - set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\src\\\\MinTray.ico")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MinTray")
set(CPACK_PACKAGE_VENDOR "Benbuck Nason")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_EXECUTABLES "MinTray" "MinTray" )
include(CPack)

if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MinTray)
endif()
