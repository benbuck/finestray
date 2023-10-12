// Copyright 2020 Benbuck Nason

#pragma once

// windows
#include <Windows.h>

void windowListStart(HWND hwnd, UINT pollMillis, void (*newWindowCallback)(HWND));
void windowListStop();
