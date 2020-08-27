// Copyright 2020 Benbuck Nason

#include "TrayIcon.h"

#include "DebugPrint.h"

volatile LONG TrayIcon::gid_ = 0;

TrayIcon::TrayIcon() { ZeroMemory(&nid_, sizeof(nid_)); }
TrayIcon::~TrayIcon() { destroy(); }

bool TrayIcon::create(HWND hwnd, UINT msg, HICON icon)
{
    if (nid_.uID) {
        DEBUG_PRINTF("attempt to re-create tray icon\n");
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
    GetWindowText(hwnd, nid_.szTip, sizeof(nid_.szTip) / sizeof(nid_.szTip[0]));
    nid_.uVersion = NOTIFYICON_VERSION;

    if (!Shell_NotifyIcon(NIM_ADD, &nid_)) {
        DEBUG_PRINTF("Shell_NotifyIcon(NIM_ADD) failed\n");
        ZeroMemory(&nid_, sizeof(nid_));
        return false;
    }

    if (!Shell_NotifyIcon(NIM_SETVERSION, &nid_)) {
        DEBUG_PRINTF("Shell_NotifyIcon(NIM_SETVERSION) failed\n");
        destroy();
        return false;
    }

    return true;
}

void TrayIcon::destroy()
{
    if (nid_.uID) {
        if (!Shell_NotifyIcon(NIM_DELETE, &nid_)) {
            DEBUG_PRINTF("Shell_NotifyIcon(NIM_DELETE) failed\n");
        }

        ZeroMemory(&nid_, sizeof(nid_));
    }
}

#if 0
void TrayIcon::refresh()
{
    nid_.uFlags = NIF_TIP;
    GetWindowText(nid_.hWnd, nid_.szTip, sizeof(nid_.szTip) / sizeof(nid_.szTip[0]));
    Shell_NotifyIcon(NIM_MODIFY, &nid_);
}
#endif
