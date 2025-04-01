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

#pragma once

// App
#include "ErrorContext.h"
#include "IconHandleWrapper.h"

// Windows
#include <Windows.h>
#include <shellapi.h>

// Standard library
#include <string>

// manages a single icon in the tray (Windows taskbar notification area)
class TrayIcon
{
public:
    TrayIcon() noexcept = default;

    ~TrayIcon() { destroy(); }

    TrayIcon(const TrayIcon &) = delete;
    TrayIcon(TrayIcon &&) = delete;
    TrayIcon & operator=(const TrayIcon &) = delete;
    TrayIcon & operator=(TrayIcon &&) = delete;

    ErrorContext create(HWND hwnd, HWND messageHwnd, UINT msg, IconHandleWrapper && icon);
    void destroy();

    void updateTip(const std::string & tip);

    inline UINT id() const { return nid_.uID; }

private:
    NOTIFYICONDATAA nid_ {};

    IconHandleWrapper icon_;
    static volatile LONG gid_;
};
