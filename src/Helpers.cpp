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

// App
#include "Helpers.h"
#include "AppName.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <dwmapi.h>

namespace
{

bool isAltTabWindow(HWND hwnd);
bool isToolWindow(HWND hwnd);
bool isCloakedWindow(HWND hwnd);

} // anonymous namespace

std::string getResourceString(unsigned int id)
{
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(nullptr);

    LPWSTR str = nullptr;
    int strLength = LoadStringW(hinstance, id, (LPWSTR)&str, 0);
    if (!strLength) {
        WARNING_PRINTF(
            "failed to load resources string %u, LoadStringA() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
        return "Error ID: " + std::to_string(id);
    }

    std::wstring wstr(str, strLength);
    return StringUtility::wideStringToString(wstr);
}

std::string getWindowText(HWND hwnd)
{
    std::string text;
    int len = GetWindowTextLengthA(hwnd);
    if (!len) {
        return std::string();
    }

    text.resize(len + 1);
    int res = GetWindowTextA(hwnd, &text[0], (int)text.size());
    if (!res && (GetLastError() != ERROR_SUCCESS)) {
        WARNING_PRINTF(
            "failed to get window text, GetWindowTextA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return std::string();
    }

    text.resize(res); // remove nul terminator

    return text;
}

std::string getWindowClassName(HWND hwnd)
{
    std::string className;
    className.resize(256);
    int res = GetClassNameA(hwnd, &className[0], (int)className.size());
    if (!res) {
        WARNING_PRINTF(
            "failed to get window class name, GetClassNameA() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return std::string();
    }

    className.resize(res); // remove nul terminator

    return className;
}

bool isWindowUserVisible(HWND hwnd)
{
    return IsWindowVisible(hwnd) && isAltTabWindow(hwnd) && !isToolWindow(hwnd) && !isCloakedWindow(hwnd);
}

void errorMessage(unsigned int id)
{
    const std::string & err = getResourceString(id);

    ERROR_PRINTF("%s\n", err.c_str());
    if (!MessageBoxA(nullptr, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR)) {
        WARNING_PRINTF(
            "failed to display error message %#x, MessageBoxA() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
    }
}

void errorMessage(const ErrorContext & errorContext)
{
    std::string err = getResourceString(errorContext.errorId());

    if (!errorContext.errorString().empty()) {
        err += ": " + errorContext.errorString();
    }

    ERROR_PRINTF("%s\n", err.c_str());
    if (!MessageBoxA(nullptr, err.c_str(), APP_NAME, MB_OK | MB_ICONERROR)) {
        WARNING_PRINTF(
            "failed to display error message %#x, MessageBoxA() failed: %s\n",
            errorContext.errorId(),
            StringUtility::lastErrorString().c_str());
    }
}

namespace
{

// from https://devblogs.microsoft.com/oldnewthing/20071008-00/?p=24863
bool isAltTabWindow(HWND hwnd)
{
    // Start at the root owner
    HWND hwndWalk = GetAncestor(hwnd, GA_ROOTOWNER);

    // See if we are the last active visible popup
    HWND hwndTry;
    while ((hwndTry = GetLastActivePopup(hwndWalk)) != hwndTry) {
        if (IsWindowVisible(hwndTry)) {
            break;
        }
        hwndWalk = hwndTry;
    }

    return hwndWalk == hwnd;
}

bool isToolWindow(HWND hwnd)
{
    LONG_PTR exStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    return (exStyle & WS_EX_TOOLWINDOW) != 0;
}

// from https://devblogs.microsoft.com/oldnewthing/20200302-00/?p=103507
bool isCloakedWindow(HWND hwnd)
{
    BOOL isCloaked = FALSE;
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
    return SUCCEEDED(hr) && isCloaked;
}

} // anonymous namespace
