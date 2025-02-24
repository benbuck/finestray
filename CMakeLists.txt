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
    src/AboutDialog.cpp
    src/AboutDialog.h
    src/AppName.h
    src/Bitmap.cpp
    src/Bitmap.h
    src/BitmapHandleWrapper.h
    src/COMLibraryWrapper.h
    src/ContextMenu.cpp
    src/ContextMenu.h
    src/CommandLine.cpp
    src/CommandLine.h
    src/DeviceContextHandleWrapper.h
    src/File.cpp
    src/File.h
    src/Finestray.cpp
    src/Finestray.rc
    src/HandleWrapper.h
    src/Helpers.cpp
    src/Helpers.h
    src/Hotkey.cpp
    src/Hotkey.h
    src/Log.cpp
    src/Log.h
    src/MenuHandleWrapper.h
    src/MinimizedWindow.cpp
    src/MinimizedWindow.h
    src/MinimizePlacement.cpp
    src/MinimizePlacement.h
    src/Modifiers.cpp
    src/Modifiers.h
    src/Path.cpp
    src/Path.h
    src/Resource.h
    src/Settings.cpp
    src/Settings.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/StringUtility.cpp
    src/StringUtility.h
    src/TrayIcon.cpp
    src/TrayIcon.h
    src/WinEventHookHandleWrapper.h
    src/WindowHandleWrapper.h
    src/WindowIcon.cpp
    src/WindowIcon.h
    src/WindowList.cpp
    src/WindowList.h
    src/WindowMessage.h
    ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico
)

target_compile_options(Finestray
    PRIVATE
        ${COMPILE_CONFIG_OPTIONS}
)

target_compile_definitions(Finestray
    PRIVATE
        UNICODE # Windows headers use wide character by default
)

target_compile_features(Finestray
    PRIVATE
        cxx_std_20
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

set(APP_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/finestray.svg)
set(EXIT_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/emoji_u274c.svg)
set(SETTINGS_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/emoji_u2699.svg)

# the following commands require ImageMagick to be installed: https://www.imagemagick.org/

# create app icon
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico
    COMMAND magick -background none -define icon:auto-resize="16,24,32,48,64,72,96,128,256" ${APP_IMAGE_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico
)

# create app bitmap
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/app.bmp"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/app.bmp
    COMMAND magick -background none -filter point -size 16x16 ${APP_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/app.bmp
)

# create exit bitmap
add_custom_command(
    COMMENT "Converting ${EXIT_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/exit.bmp"
    DEPENDS ${EXIT_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/exit.bmp
    COMMAND magick -background none -filter point -size 16x16 ${EXIT_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/exit.bmp
)

# create settings bitmap
add_custom_command(
    COMMENT "Converting ${SETTINGS_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/settings.bmp"
    DEPENDS ${SETTINGS_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/settings.bmp
    COMMAND magick -background none -filter point -size 16x16 ${SETTINGS_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/settings.bmp
)

# create installer header image (used at top of most installer screens)
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/installer_header.bmp"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/installer_header.bmp
    COMMAND magick -size 150x57 -define gradient:direction=east gradient:grey50-white ${CMAKE_CURRENT_BINARY_DIR}/installer_header.bmp
    COMMAND magick composite -background none -geometry 40x40+15+8 ${APP_IMAGE_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/installer_header.bmp BMP3:${CMAKE_CURRENT_BINARY_DIR}/installer_header.bmp
)

# create installer welcome image (used at left side of installer welcome screen)
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/installer_welcome.bmp"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/installer_welcome.bmp
    COMMAND magick -size 164x314 -define gradient:direction=east gradient:grey50-white ${CMAKE_CURRENT_BINARY_DIR}/installer_welcome.bmp
    COMMAND magick composite -background none -geometry 100x100+15+45 ${APP_IMAGE_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/installer_welcome.bmp BMP3:${CMAKE_CURRENT_BINARY_DIR}/installer_welcome.bmp
)

# create icon for README.md
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_SOURCE_DIR}/src/images/icon.png"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/images/icon.png
    COMMAND magick -background none -define png:exclude-chunks=date,time -filter point -size 16x16 ${APP_IMAGE_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/images/icon.png
)

add_custom_target(
    generate-images
    ALL
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico
        ${CMAKE_CURRENT_BINARY_DIR}/app.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/exit.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/settings.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/installer_header.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/installer_welcome.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/images/icon.png
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
set(CPACK_NSIS_DEFINES "RequestExecutionLevel user")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMSTARTUP\\\\Finestray.lnk'")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "Exec '\\\"$INSTDIR\\\\Finestray.exe\\\"'")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$INSTDIR\\\\Finestray.json'")
set(CPACK_NSIS_INSTALLED_ICON_NAME "Finestray.exe")
set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_CURRENT_BINARY_DIR}\\\\installer_header.bmp")
set(CPACK_NSIS_HELP_LINK "https://github.com/benbuck/finestray/")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_BINARY_DIR}\\\\installer_welcome.bmp")
set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_BINARY_DIR}\\\\installer_welcome.bmp")
include(CPack)

if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Finestray)
endif()
