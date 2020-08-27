// Copyright 2020 Benbuck Nason

#pragma once

#include <Windows.h>

#define WM_TRAYWINDOW (WM_USER + 1)

void trayWindowMinimize(HWND hwnd, HWND messageWnd);
void trayWindowRestore(HWND hwnd);
void trayWindowClose(HWND hwnd);
// void trayWindowRefresh(HWND hwnd);
void trayWindowAddAll();
void trayWindowRestoreAll();
HWND trayWindowGetFromID(UINT id);
HWND trayWindowGetLast();
