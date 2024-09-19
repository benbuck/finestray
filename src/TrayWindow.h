// Copyright 2020 Benbuck Nason

#pragma once

// Windows
#include <Windows.h>

#define WM_TRAYWINDOW (WM_USER + 1)

namespace TrayWindow
{

void minimize(HWND hwnd, HWND messageWnd);
void restore(HWND hwnd);
void close(HWND hwnd);
void addAll();
void restoreAll();
HWND getFromID(UINT id);
HWND getLast();
bool exists(HWND hwnd);

} // namespace TrayWindow
