# Copyright 2020 Benbuck Nason

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    cmake_policy(SET CMP0117 NEW) # /GR (RTTI) is not added to CMAKE_CXX_FLAGS by default
endif()

project(MinTray
    VERSION 0.1
)

set(COMPILE_CONFIG_OPTIONS
    $<$<CXX_COMPILER_ID:MSVC>:
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
        -fno-exceptions
        -fno-rtti
        -Wall
        -Werror

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

add_library(cJSON
    src/cJSON/cJSON.c
    src/cJSON/cJSON.h
)

target_include_directories(cJSON
    INTERFACE
        src/cJSON
)

target_compile_options(cJSON
    PRIVATE
        ${COMPILE_CONFIG_OPTIONS}
)

add_library(argh
    INTERFACE
        src/argh/argh.h
)

target_include_directories(argh
    INTERFACE
    src/argh
)

add_executable(MinTray WIN32
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
    src/StringUtilities.cpp
    src/StringUtilities.h
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
)

install(
    TARGETS MinTray
    RUNTIME DESTINATION .
    COMPONENT Runtime
)

set(CPACK_GENERATOR "NSIS64")
set(CPACK_NSIS_INSTALLED_ICON_NAME "MinTray.exe")
set(CPACK_NSIS_HELP_LINK "https://github.com/benbuck/mintray/")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MinTray")
set(CPACK_PACKAGE_VENDOR "Benbuck Nason")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
include(CPack)

if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MinTray)
endif()
