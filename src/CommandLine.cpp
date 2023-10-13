// Copyright 2020 Benbuck Nason

#include "CommandLine.h"

// MinTray
#include "DebugPrint.h"

// windows
#include <Windows.h>

namespace CommandLine
{

void getArgs(int * argc, char *** argv)
{
    *argc = 0;
    *argv = nullptr;

    LPWSTR commandLine = GetCommandLineW();
    if (!commandLine) {
        DEBUG_PRINTF("GetCommandLine() failed: %u\n", GetLastError());
        return;
    }

    wchar_t ** wargv = CommandLineToArgvW(commandLine, argc);
    if (!*argc || !wargv) {
        DEBUG_PRINTF("CommandLineToArgvW() failed: %u\n", GetLastError());
        return;
    }

    *argv = new char *[*argc + 1];
    if (!*argv) {
        *argc = 0;
        if (LocalFree(wargv)) {
            DEBUG_PRINTF("LocalFree() failed: %u\n", GetLastError());
        }
        return;
    }
    (*argv)[*argc] = nullptr;

    bool failure = false;

    for (int a = 0; !failure && (a < *argc); a++) {
        int ret = WideCharToMultiByte(CP_UTF8, 0, wargv[a], -1, NULL, 0, NULL, NULL);
        if (ret <= 0) {
            DEBUG_PRINTF("WideCharToMultiByte() failed: %u\n", GetLastError());
            failure = true;
            break;
        }

        size_t argLen = (size_t)ret + 1;
        char * arg = new char[argLen];

        ret = WideCharToMultiByte(CP_UTF8, 0, wargv[a], -1, arg, (int)argLen, NULL, NULL);
        if (ret <= 0) {
            DEBUG_PRINTF("WideCharToMultiByte() failed: %u\n", GetLastError());
            failure = true;
            delete[] arg;
            break;
        }

        (*argv)[a] = arg;
    }

    if (LocalFree(wargv)) {
        DEBUG_PRINTF("LocalFree() failed: %u\n", GetLastError());
    }

    if (failure) {
        delete[] * argv;
        *argv = nullptr;
        *argc = 0;
    }
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
