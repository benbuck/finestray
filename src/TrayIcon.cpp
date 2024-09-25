// Copyright 2020 Benbuck Nason

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

bool TrayIcon::create(HWND hwnd, UINT msg, HICON icon)
{
    if (nid_.uID) {
        DEBUG_PRINTF("attempt to re-create tray icon %u\n", nid_.uID);
        return false;
    }

    LONG id = InterlockedIncrement(&gid_);

    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = NOTIFYICONDATA_V2_SIZE;
    nid_.hWnd = hwnd;
    nid_.uID = (UINT)id;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = msg;
    nid_.hIcon = icon;
    if (!GetWindowTextA(hwnd, nid_.szTip, sizeof(nid_.szTip) / sizeof(nid_.szTip[0]))) {
        DEBUG_PRINTF("could not window text, GetWindowTextA() failed: %s\n", StringUtility::lastErrorString().c_str());
    }
    nid_.uVersion = NOTIFYICON_VERSION;

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
