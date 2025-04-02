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

include(FetchContent)
find_package(Patch REQUIRED)

set(CJSON_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_UNINSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_UTILS OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_VERSION_SO OFF CACHE BOOL "" FORCE)
set(ENABLE_TARGET_EXPORT OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    cJSON
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG        v1.7.18
    PATCH_COMMAND "${Patch_EXECUTABLE}" -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/cJSON.patch
    UPDATE_DISCONNECTED 1
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(cJSON)

target_include_directories(cjson INTERFACE ${cJSON_SOURCE_DIR})

# could potentially be removed if cJSON fixes all warnings
# see https://github.com/DaveGamble/cJSON/issues/894 for one such issue
target_compile_options(cjson
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W0> # disable warnings
    $<$<CXX_COMPILER_ID:Clang>:-w> # disable warnings
)

add_library(cJSON ALIAS cjson)
