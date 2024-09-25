// Copyright 2020 Benbuck Nason

#include "CommandLine.h"

// App
#include "DebugPrint.h"
#include "StringUtility.h"

// Windows
#include <Windows.h>

CommandLine::CommandLine()
    : args_()
    , argv_()
{
}

bool CommandLine::parse()
{
    LPWSTR commandLine = GetCommandLineW();
    if (!commandLine) {
        DEBUG_PRINTF("GetCommandLine() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    DEBUG_PRINTF("Got command line: %ls\n", commandLine);

    int argc = 0;
    wchar_t ** wargv = CommandLineToArgvW(commandLine, &argc);
    if (!argc || !wargv) {
        DEBUG_PRINTF("CommandLineToArgvW() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    args_.resize(argc);
    argv_.resize(argc);

    for (size_t a = 0; a < args_.size(); ++a) {
        args_[a] = StringUtility::wideStringToString(wargv[a]).c_str();
        if (args_[a].empty()) {
            args_.clear();
            argv_.clear();
            return false;
        }
        argv_[a] = args_[a].c_str();
    }

    if (LocalFree(wargv)) {
        DEBUG_PRINTF("LocalFree() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return true;
}
