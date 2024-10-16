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

#include "TrayIcon.h"

// App
#include "DebugPrint.h"
#include "StringUtility.h"

volatile LONG TrayIcon::gid_ = 0;

TrayIcon::TrayIcon()
{
    ZeroMemory(&nid_, sizeof(nid_));
}

TrayIcon::~TrayIcon()
{
    destroy();
}

bool TrayIcon::create(HWND hwnd, UINT msg, HICON hicon)
{
    if (nid_.uID) {
        DEBUG_PRINTF("attempt to re-create tray icon %u\n", nid_.uID);
        return false;
    }

    LONG id = InterlockedIncrement(&gid_);

    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = NOTIFYICONDATA_V3_SIZE;
    nid_.hWnd = hwnd;
    nid_.uID = (UINT)id;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = msg;
    nid_.hIcon = hicon;
    nid_.uVersion = NOTIFYICON_VERSION;

    if (!GetWindowTextA(hwnd, nid_.szTip, sizeof(nid_.szTip) / sizeof(nid_.szTip[0]))) {
        DEBUG_PRINTF("could not window text, GetWindowTextA() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    if (!Shell_NotifyIconA(NIM_ADD, &nid_)) {
        DEBUG_PRINTF("could not add tray icon, Shell_NotifyIcon() failed: %s\n", StringUtility::lastErrorString().c_str());
        ZeroMemory(&nid_, sizeof(nid_));
        return false;
    }

    if (!Shell_NotifyIconA(NIM_SETVERSION, &nid_)) {
        DEBUG_PRINTF(
            "could not set tray icon version, Shell_NotifyIcon() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        destroy();
        return false;
    }

    return true;
}

void TrayIcon::destroy()
{
    if (nid_.uID) {
        if (!Shell_NotifyIconA(NIM_DELETE, &nid_)) {
            DEBUG_PRINTF(
                "could not destroy tray icon, Shell_NotifyIcon() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        ZeroMemory(&nid_, sizeof(nid_));
    }
}
