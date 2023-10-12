// Copyright 2020 Benbuck Nason

#pragma once

#include <Windows.h>

void windowListStart(HWND hwnd, UINT pollMillis, void (*newWindowCallback)(HWND));
void windowListStop();
