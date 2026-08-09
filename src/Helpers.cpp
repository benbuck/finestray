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
#include "AppInfo.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <dwmapi.h>

namespace
{

bool isToolWindow(HWND hwnd) noexcept;
bool isCloakedWindow(HWND hwnd) noexcept;
bool isNoActivateWindow(HWND hwnd) noexcept;
bool isOwnedWindow(HWND hwnd) noexcept;

} // anonymous namespace

HINSTANCE getInstance() noexcept
{
    return GetModuleHandle(nullptr);
}

std::string getResourceString(unsigned int id)
{
    HINSTANCE hinstance = getInstance();

    LPWSTR str = nullptr;
    const int strLength = LoadStringW(hinstance, id, reinterpret_cast<LPWSTR>(&str), 0);
    if (!strLength) {
        WARNING_PRINTF(
            "failed to load resources string %u, LoadStringA() failed: %s\n",
            id,
            StringUtility::lastErrorString().c_str());
        return "Error ID: " + std::to_string(id);
    }

    const std::wstring wstr(str, narrow_cast<size_t>(strLength));
    return StringUtility::wideStringToString(wstr);
}

bool isWindowStealth(HWND hwnd) noexcept
{
    return isToolWindow(hwnd) || isCloakedWindow(hwnd) || isNoActivateWindow(hwnd) || isOwnedWindow(hwnd);
}

bool isWindowUserVisible(HWND hwnd) noexcept
{
    return IsWindowVisible(hwnd) && !isWindowStealth(hwnd);
}

bool isUwpWindow(HWND hwnd) noexcept
{
    char className[MAX_CLASS_NAME] = {};
    if (!GetClassNameA(hwnd, className, narrow_cast<int>(sizeof(className)))) {
        return false;
    }
    return _stricmp(className, "ApplicationFrameWindow") == 0;
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

bool isToolWindow(HWND hwnd) noexcept
{
    LONG_PTR const exStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    return (narrow_cast<UINT_PTR>(exStyle) & WS_EX_TOOLWINDOW) != 0;
}

// from https://devblogs.microsoft.com/oldnewthing/20200302-00/?p=103507
bool isCloakedWindow(HWND hwnd) noexcept
{
    BOOL isCloaked = FALSE;
    HRESULT const hr = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
    return SUCCEEDED(hr) && isCloaked;
}

// The taskbar does not show windows with WS_EX_NOACTIVATE
bool isNoActivateWindow(HWND hwnd) noexcept
{
    const LONG_PTR exStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    return (narrow_cast<UINT_PTR>(exStyle) & WS_EX_NOACTIVATE) != 0;
}

// The taskbar does not show owned windows unless WS_EX_APPWINDOW is set
bool isOwnedWindow(HWND hwnd) noexcept
{
    const LONG_PTR exStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);
    if ((narrow_cast<UINT_PTR>(exStyle) & WS_EX_APPWINDOW) != 0) {
        return false;
    }
    return GetWindow(hwnd, GW_OWNER) != nullptr;
}

} // anonymous namespace
