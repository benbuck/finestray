# Copyright 2020 Benbuck Nason

include(FetchContent)

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
