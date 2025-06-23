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

// Finestray
#include "WindowInfo.h"
#include "HandleWrapper.h"
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <Windows.h>
#include <shellapi.h>

WindowInfo::WindowInfo(HWND hwnd)
{
    className_.resize(256);
    int res = GetClassNameA(hwnd, className_.data(), narrow_cast<int>(className_.size()));
    if (!res) {
        WARNING_PRINTF(
            "failed to get window class name, GetClassNameA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        className_.clear();
    } else {
        className_.resize(narrow_cast<size_t>(res)); // remove nul terminator
    }

    char executableFullPath[MAX_PATH] = {};
    DWORD processID = 0;
    if (!GetWindowThreadProcessId(hwnd, &processID)) {
        WARNING_PRINTF("GetWindowThreadProcessId() failed: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        const HandleWrapper process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID));
        if (!process) {
            WARNING_PRINTF("OpenProcess() failed: %s\n", StringUtility::lastErrorString().c_str());
        } else {
            if (!GetModuleFileNameExA(static_cast<HANDLE>(process), nullptr, executableFullPath, sizeof(executableFullPath))) {
                WARNING_PRINTF("GetModuleFileNameExA() failed: %s\n", StringUtility::lastErrorString().c_str());
            } else {
                executable_ = executableFullPath;
            }
        }
    }

    title_ = getTitle(hwnd);
}

std::string WindowInfo::getTitle(HWND hwnd)
{
    const int len = GetWindowTextLengthA(hwnd);
    if (!len) {
        DWORD error = GetLastError();
        if (error != ERROR_SUCCESS) {
            WARNING_PRINTF(
                "failed to get window %#x title length, GetWindowTextLengthA() failed: %s\n",
                hwnd,
                StringUtility::errorToString(error).c_str());
        }
        return {};
    }

    std::string title;
    title.resize(static_cast<size_t>(len) + 1);
    const int res = GetWindowTextA(hwnd, title.data(), narrow_cast<int>(title.size()));
    if (!res) {
        DWORD error = GetLastError();
        if (error != ERROR_SUCCESS) {
            WARNING_PRINTF(
                "failed to get window %#x title, GetWindowTextA() failed: %s\n",
                hwnd,
                StringUtility::errorToString(error).c_str());
            return {};
        }
    }

    title.resize(narrow_cast<size_t>(res)); // remove nul terminator

    return title;
}
