// Copyright 2020 Benbuck Nason

#pragma once

#include <Windows.h>

struct Settings
{
    Settings();
    ~Settings();

    void parseCommandLine();
    void parseJson(const char * json);
    void addAutotray(const char * className);

    bool shouldExit_;

    struct Autotray
    {
        WCHAR * className_;
    };
    Autotray * autotray_;
    size_t autotraySize_;
};
