# Copyright 2020 Benbuck Nason

include(FetchContent)

FetchContent_Declare(
    argh
    GIT_REPOSITORY https://github.com/adishavit/argh.git
    GIT_TAG        v1.3.2
)

FetchContent_MakeAvailable(argh)

target_include_directories(argh INTERFACE ${argh_SOURCE_DIR})
