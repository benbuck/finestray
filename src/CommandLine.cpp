// Copyright 2020 Benbuck Nason

#include "CommandLine.h"

// MinTray
#include "DebugPrint.h"

// windows
#include <Windows.h>

// standard library
#include <algorithm>

char * wideStringToString(const wchar_t * ws)
{
    int ret = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if (ret <= 0) {
        DEBUG_PRINTF("WideCharToMultiByte() failed: %u\n", GetLastError());
        return nullptr;
    }

    size_t len = (size_t)ret + 1;
    char * s = new char[len];

    ret = WideCharToMultiByte(CP_UTF8, 0, ws, -1, s, (int)len, nullptr, nullptr);
    if (ret <= 0) {
        DEBUG_PRINTF("WideCharToMultiByte() failed: %u\n", GetLastError());
        delete[] s;
        return nullptr;
    }

    return s;
}

namespace CommandLine
{

bool getArgs(int * argc, char *** argv)
{
    *argc = 0;
    *argv = nullptr;

    LPWSTR commandLine = GetCommandLineW();
    if (!commandLine) {
        DEBUG_PRINTF("GetCommandLine() failed: %u\n", GetLastError());
        return false;
    }

    wchar_t ** wargv = CommandLineToArgvW(commandLine, argc);
    if (!*argc || !wargv) {
        DEBUG_PRINTF("CommandLineToArgvW() failed: %u\n", GetLastError());
        return false;
    }

    *argv = new char *[(size_t)*argc + 1];
    if (!*argv) {
        *argc = 0;
        if (LocalFree(wargv)) {
            DEBUG_PRINTF("LocalFree() failed: %u\n", GetLastError());
        }
        return false;
    }

    std::fill_n(*argv, *argc + 1, nullptr);

    for (int a = 0; a < *argc; ++a) {
        (*argv)[a] = wideStringToString(wargv[a]);
        if (!(*argv)[a]) {
            freeArgs(*argc, *argv);
            *argv = nullptr;
            *argc = 0;
            return false;
        }
    }

    if (LocalFree(wargv)) {
        DEBUG_PRINTF("LocalFree() failed: %u\n", GetLastError());
    }

    return true;
}

void freeArgs(int argc, char ** argv)
{
    if (argv) {
        for (int a = 0; a < argc; ++a) {
            delete[] argv[a];
        }
        delete[] argv;
    }
}

} // namespace CommandLine
