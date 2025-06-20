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
#include "TrayIcon.h"
#include "Helpers.h"
#include "IconHandleWrapper.h"
#include "Log.h"
#include "Resource.h"
#include "StringUtility.h"

// Windows
#include <combaseapi.h>

// Standard library
#include <map>

namespace
{

typedef std::map<UINT, HWND> IDMap;

IDMap idMap_;
volatile LONG gid_ = 0;

} // anonymous namespace

ErrorContext TrayIcon::create(HWND hwnd, HWND messageHwnd, UINT msg, IconHandleWrapper && icon)
{
    LONG const gid = InterlockedIncrement(&gid_);

    if (nid_.uID) {
        WARNING_PRINTF("tray icon already created, destroying first\n");
        destroy();
    }

    DEBUG_PRINTF("creating tray icon %u\n", gid);

    icon_ = std::move(icon);

    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = NOTIFYICONDATA_V3_SIZE;
    nid_.hWnd = messageHwnd;
    nid_.uID = narrow_cast<UINT>(gid);
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_GUID;
    nid_.uCallbackMessage = msg;
    nid_.hIcon = icon_ ? icon_ : LoadIcon(nullptr, IDI_APPLICATION);
    nid_.uVersion = NOTIFYICON_VERSION;

    if (!GetWindowTextA(hwnd, nid_.szTip, sizeof(nid_.szTip) / sizeof(nid_.szTip[0])) &&
        (GetLastError() != ERROR_SUCCESS)) {
        WARNING_PRINTF("could not window text, GetWindowTextA() failed: %s\n", StringUtility::lastErrorString().c_str());
        nid_.uFlags &= ~static_cast<UINT>(NIF_TIP);
    }

    if (FAILED(CoCreateGuid(&nid_.guidItem))) {
        WARNING_PRINTF(
            "could not create tray icon guid, CoCreateGuid() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        nid_.uFlags &= ~static_cast<UINT>(NIF_GUID);
    }

    if (!Shell_NotifyIconA(NIM_ADD, &nid_)) {
        const std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("could not add tray icon, Shell_NotifyIcon() failed: %s\n", lastErrorString.c_str());
        ZeroMemory(&nid_, sizeof(nid_));
        return { IDS_ERROR_CREATE_TRAY_ICON, lastErrorString + " (NIM_ADD)" };
    }

    if (!Shell_NotifyIconA(NIM_SETVERSION, &nid_)) {
        const std::string lastErrorString = StringUtility::lastErrorString();
        WARNING_PRINTF("could not set tray icon version, Shell_NotifyIcon() failed: %s\n", lastErrorString.c_str());
        destroy();
        return { IDS_ERROR_CREATE_TRAY_ICON, lastErrorString + " (NIM_SETVERSION)" };
    }

    idMap_[nid_.uID] = hwnd;

    return {};
}

void TrayIcon::destroy() noexcept
{
    if (nid_.uID) {
        DEBUG_PRINTF("destroying tray icon %u\n", nid_.uID);

        idMap_.erase(nid_.uID);

        if (!Shell_NotifyIconA(NIM_DELETE, &nid_)) {
            WARNING_PRINTF("could not destroy tray icon, Shell_NotifyIcon() failed: %lu\n", GetLastError());
        }

        ZeroMemory(&nid_, sizeof(nid_));
    }
}

void TrayIcon::updateTip(const std::string & tip)
{
    if (nid_.uID) {
        DEBUG_PRINTF("updating tray icon %u tip to '%s'\n", nid_.uID, tip.c_str());
        constexpr size_t tipMaxSize = sizeof(nid_.szTip) / sizeof(nid_.szTip[0]);
        strncpy_s(nid_.szTip, tip.c_str(), tipMaxSize - 1);
        if (!Shell_NotifyIconA(NIM_MODIFY, &nid_)) {
            WARNING_PRINTF(
                "could not update tray icon tip, Shell_NotifyIcon() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }
    }
}

HWND TrayIcon::getWindowFromID(UINT id)
{
    const IDMap::iterator it = idMap_.find(id);
    if (it != idMap_.end()) {
        return it->second;
    }
    return nullptr;
}
