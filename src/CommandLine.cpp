// Copyright 2020 Benbuck Nason
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "CommandLine.h"

// App
#include "Log.h"
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
        WARNING_PRINTF("GetCommandLine() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    DEBUG_PRINTF("Got command line: %ls\n", commandLine);

    int argc = 0;
    wchar_t ** wargv = CommandLineToArgvW(commandLine, &argc);
    if (!argc || !wargv) {
        WARNING_PRINTF("CommandLineToArgvW() failed: %s\n", StringUtility::lastErrorString().c_str());
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
        WARNING_PRINTF("LocalFree() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    return true;
}
