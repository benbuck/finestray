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

// Windows
#include <Windows.h>

#define WM_TRAYWINDOW (WM_USER + 1)
#define WM_SHOWSETTINGS (WM_USER + 2)

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
