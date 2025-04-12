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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIGURATION_TYPES Debug Release Analyze)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_MSVC_RUNTIME_CHECKS "$<$<CONFIG:Debug>:PossibleDataLoss;UninitializedVariable>")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    cmake_policy(set CMP0117 NEW) # /GR (RTTI) is not added to CMAKE_CXX_FLAGS by default
endif()

project(Finestray
    VERSION 0.3
)

add_executable(Finestray WIN32
    src/AboutDialog.cpp
    src/AboutDialog.h
    src/AppName.h
    src/Bitmap.cpp
    src/Bitmap.h
    src/BitmapHandleWrapper.h
    src/BrushHandleWrapper.h
    src/CJsonWrapper.h
    src/COMLibraryWrapper.h
    src/ContextMenu.cpp
    src/ContextMenu.h
    src/DeviceContextHandleWrapper.h
    src/ErrorContext.h
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
    src/TrayEvent.cpp
    src/TrayEvent.h
    src/TrayIcon.cpp
    src/TrayIcon.h
    src/WindowHandleWrapper.h
    src/WindowIcon.cpp
    src/WindowIcon.h
    src/WindowInfo.cpp
    src/WindowInfo.h
    src/WindowList.cpp
    src/WindowList.h
    src/WindowMessage.h
    src/WinEventHookHandleWrapper.h
    ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico
)

set_target_properties(Finestray
    PROPERTIES
        VS_USER_PROPS ${CMAKE_CURRENT_SOURCE_DIR}/analysis.props
        $<$<CONFIG:Analyze>:
            CXX_CLANG_TIDY "$clang-tidy;--extra-arg=/EHsc;--fix;--fix-errors"
        >
)

#set_property(TARGET Finestray PROPERTY CXX_CLANG_TIDY $<$<CONFIG:Analyze>:"clang-tidy;--extra-arg=/EHsc;--fix;--fix-errors">)
#set_property(TARGET Finestray PROPERTY VS_USER_PROPS $<$<CONFIG:Analyze>:"${CMAKE_CURRENT_SOURCE_DIR}/analysis.props">)

target_compile_options(Finestray
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            /GR- # disable run-time type information (RTTI)
            /GS # enable security checks
            /guard:cf # control flow guard
            /W4 # maximum warning levels
            /WX # treat warnings as errors

            # Debug compile options
            $<$<CONFIG:Debug>:
                /Od # don't optimize
                /RTC1 # enable basic run-time checks
                /Zi # enable debug info in pdb format
            >

            # Release compile options
            $<$<CONFIG:Release>:
                /O2 # enable maximum speed optimizations
                /Ot # favor fast code
                /GL # enable whole program optimization
                /Gw # enable global data optimization
            >

            # Analyze compile options
            $<$<CONFIG:Analyze>:
                /analyze # enable static analysis
                /wd26485 # No array to pointer decay
                /wd26490 # Don't use reinterpret_cast
            >
        >

        $<$<CXX_COMPILER_ID:Clang>:
            # disable run-time type information (RTTI)
            $<$<CXX_COMPILER_FRONTEND_VARIANT:GNU>: -fno-rtti >
            $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>: /GR- >

            -Wall # enable all warnings
            -Wextra # enable extra warnings
            -Weverything # enable all warnings
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-covered-switch-default
            -Wno-exit-time-destructors
            -Wno-global-constructors
            -Wno-gnu-zero-variadic-macro-arguments
            -Wno-unsafe-buffer-usage
            -Werror # treat warnings as errors

            # Debug compile options
            $<$<CONFIG:Debug>:
                # don't optimize
                $<$<CXX_COMPILER_FRONTEND_VARIANT:GNU>: -O0 >
                $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>: /Od >

                -g # enable debug info

                #-fsanitize=address # enable address sanitizer
                #-fsanitize=integer # enable integer sanitizer
                #-fsanitize=undefined # enable undefined behavior sanitizer
            >

            # Release compile options
            $<$<CONFIG:Release>:
                # enable maximum speed optimizations
                $<$<CXX_COMPILER_FRONTEND_VARIANT:GNU>: -O3 >
                $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>: /Os >

                -flto # enable link time optimization
            >

            # Analyze compile options
            $<$<CONFIG:Analyze>:
                #--analyze # enable static analysis
                -Xclang=-analyzer-output=text
                -Xclang=-analyzer-checker=core
                -Xclang=-analyzer-checker=cplusplus
                -Xclang=-analyzer-checker=deadcode
                -Xclang=-analyzer-checker=nullability
                -Xclang=-analyzer-checker=optin.core
                -Xclang=-analyzer-checker=optin.cplusplus
                -Xclang=-analyzer-checker=optin.performance
                -Xclang=-analyzer-checker=optin.portability
                -Xclang=-analyzer-checker=security
                -Xclang=-analyzer-checker=unix
                -Xclang=-analyzer-checker=valist
            >
        >
)

target_compile_definitions(Finestray
    PRIVATE
        UNICODE # Windows headers use wide character by default
        NOMINMAX # Windows headers define min and max macros
        WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers

        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1> # enable iterator debugging
        $<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0> # disable iterator debugging
)

target_compile_features(Finestray
    PRIVATE
        cxx_std_20
)

target_link_options(Finestray
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            /GUARD:CF # control flow guard

            # Release link options
            $<$<CONFIG:Release>:
                /LTCG # link time code generation
            >
        >

        $<$<CXX_COMPILER_ID:Clang>:
            # Debug link options
            $<$<CONFIG:Debug>:
                #-fsanitize=address # enable address sanitizer
                #-fsanitize=integer # enable integer sanitizer
                #-fsanitize=undefined # enable undefined behavior sanitizer
            >

            # Release link options
            $<$<CONFIG:Release>:
                #-flto # link time optimization
            >

            # Analyze link options
            $<$<CONFIG:Analyze>:
            >
        >
)

include(cJSON.cmake)

target_link_libraries(Finestray
    PRIVATE
        cJSON

        Comctl32.lib
        ShLwApi.lib
        dwmapi.lib
)

set(APP_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/finestray.svg)
set(SCREENSHOT_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/settings-window.png)
set(EXIT_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/emoji_u274c.svg)
set(SETTINGS_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/emoji_u2699.svg)
set(MINIMIZE_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/emoji_u2198.svg)
set(RESTORE_IMAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/images/emoji_u2196.svg)

# the following commands require ImageMagick to be installed: https://www.imagemagick.org/

# create app icon
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico
    COMMAND magick -background none -define icon:auto-resize="16,24,32,48,64,72,96,128,256" ${APP_IMAGE_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/Finestray.ico
)

# create app store images
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/logo-1440x2160.png"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/logo-1440x2160.png
    COMMAND magick -background none ${APP_IMAGE_SOURCE} -resize 1440x1440 -gravity center -extent 1440x2160 ${CMAKE_CURRENT_BINARY_DIR}/logo-1440x2160.png
)
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/logo-2160x2160.png"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/logo-2160x2160.png
    COMMAND magick -background none ${APP_IMAGE_SOURCE} -resize 2160x2160 ${CMAKE_CURRENT_BINARY_DIR}/logo-2160x2160.png
)
add_custom_command(
    COMMENT "Converting ${SCREENSHOT_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/screenshot-3840x2160.png"
    DEPENDS ${SCREENSHOT_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/screenshot-3840x2160.png
    COMMAND magick -background none ${SCREENSHOT_IMAGE_SOURCE} -resize 3840x2160 -gravity center -extent 3840x2160 ${CMAKE_CURRENT_BINARY_DIR}/screenshot-3840x2160.png
)

# create app bitmap
add_custom_command(
    COMMENT "Converting ${APP_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/app.bmp"
    DEPENDS ${APP_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/app.bmp
    COMMAND magick -size 16x16 -background none -filter point ${APP_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/app.bmp
)

# create minimize bitmap
add_custom_command(
    COMMENT "Converting ${MINIMIZE_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/minimize.bmp"
    DEPENDS ${MINIMIZE_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/minimize.bmp
    COMMAND magick -size 16x16 -background none -filter point ${MINIMIZE_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/minimize.bmp
)

# create restore bitmap
add_custom_command(
    COMMENT "Converting ${RESTORE_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/restore.bmp"
    DEPENDS ${RESTORE_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/restore.bmp
    COMMAND magick -size 16x16 -background none -filter point ${RESTORE_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/restore.bmp
)

# create settings bitmap
add_custom_command(
    COMMENT "Converting ${SETTINGS_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/settings.bmp"
    DEPENDS ${SETTINGS_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/settings.bmp
    COMMAND magick -size 16x16 -background none -filter point ${SETTINGS_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/settings.bmp
)

# create exit bitmap
add_custom_command(
    COMMENT "Converting ${EXIT_IMAGE_SOURCE} to ${CMAKE_CURRENT_BINARY_DIR}/exit.bmp"
    DEPENDS ${EXIT_IMAGE_SOURCE}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/exit.bmp
    COMMAND magick -size 16x16 -background none -filter point ${EXIT_IMAGE_SOURCE} -define bmp3:alpha=true BMP3:${CMAKE_CURRENT_BINARY_DIR}/exit.bmp
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
        ${CMAKE_CURRENT_BINARY_DIR}/logo-1440x2160.png
        ${CMAKE_CURRENT_BINARY_DIR}/logo-2160x2160.png
        ${CMAKE_CURRENT_BINARY_DIR}/screenshot-3840x2160.png
        ${CMAKE_CURRENT_BINARY_DIR}/app.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/minimize.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/restore.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/settings.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/exit.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/installer_header.bmp
        ${CMAKE_CURRENT_BINARY_DIR}/installer_welcome.bmp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/images/icon.png
)

install(
    TARGETS Finestray
    RUNTIME
    DESTINATION .
)

set(CPACK_GENERATOR "NSIS64")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Finestray")
set(CPACK_PACKAGE_VENDOR "Benbuck Nason")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "Finestray" "Finestray" )

# NSIS options
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMSTARTUP\\\\Finestray.lnk' '$INSTDIR\\\\Finestray.exe'")
set(CPACK_NSIS_DEFINES_LIST
    "RequestExecutionLevel user"
    "InstallDir \\\"$LOCALAPPDATA\\\\${CPACK_PACKAGE_INSTALL_DIRECTORY}\\\""
)
string (REPLACE ";" " \n" CPACK_NSIS_DEFINES "${CPACK_NSIS_DEFINES_LIST}")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMSTARTUP\\\\Finestray.lnk'")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "Exec '\\\"$INSTDIR\\\\Finestray.exe\\\"'")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$INSTDIR\\\\Finestray.json'\nDelete '$INSTDIR\\\\Finestray.log'")
set(CPACK_NSIS_INSTALLED_ICON_NAME "Finestray.exe")
set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_CURRENT_BINARY_DIR}\\\\installer_header.bmp")
set(CPACK_NSIS_HELP_LINK "https://github.com/benbuck/finestray/")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_BINARY_DIR}\\\\installer_welcome.bmp")
set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_BINARY_DIR}\\\\installer_welcome.bmp")

include(CPack)

if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Finestray)
endif()
