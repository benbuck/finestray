// Copyright 2020 Benbuck Nason

#pragma once

// windows
#include <Windows.h>
#include <shellapi.h>

// manages a single icon in the tray (Windows taskbar notification area)
class TrayIcon
{
public:
    TrayIcon();
    ~TrayIcon();

    bool create(HWND hwnd, UINT msg, HICON icon);
    void destroy();

    inline UINT id() const { return nid_.uID; }
    inline HWND hwnd() const { return nid_.hWnd; }

private:
    TrayIcon(const TrayIcon &) = delete;
    TrayIcon & operator=(const TrayIcon &) = delete;

    NOTIFYICONDATA nid_;

    static volatile LONG gid_;
};
