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

#include "WindowInfo.h"

// Finestray
#include "HandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <CommCtrl.h>
#include <Psapi.h>
#include <Windows.h>
#include <shellapi.h>

WindowInfo::WindowInfo(HWND hwnd)
    : hwnd_(hwnd)
{
    className_.resize(256);
    int res = GetClassNameA(hwnd, className_.data(), static_cast<int>(className_.size()));
    if (!res) {
        WARNING_PRINTF(
            "failed to get window class name, GetClassNameA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        className_.clear();
    } else {
        className_.resize(res); // remove nul terminator
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
            if (!GetModuleFileNameExA(static_cast<HANDLE>(process), nullptr, executableFullPath, MAX_PATH)) {
                WARNING_PRINTF("GetModuleFileNameA() failed: %s\n", StringUtility::lastErrorString().c_str());
            } else {
                executable_ = executableFullPath;
            }
        }
    }

    const int len = GetWindowTextLengthA(hwnd);
    if (!len) {
        if (GetLastError() != ERROR_SUCCESS) {
            WARNING_PRINTF(
                "failed to get window text length, GetWindowTextLengthA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        } else {
            title_.clear(); // no title
        }
    } else {
        title_.resize(static_cast<size_t>(len) + 1);
        res = GetWindowTextA(hwnd, title_.data(), static_cast<int>(title_.size()));
        if (!res && (GetLastError() != ERROR_SUCCESS)) {
            WARNING_PRINTF(
                "failed to get window text, GetWindowTextA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
            title_.clear();
        } else {
            title_.resize(res); // remove nul terminator
        }
    }
}
