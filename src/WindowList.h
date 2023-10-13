// Copyright 2020 Benbuck Nason

#pragma once

// windows
#include <Windows.h>

namespace WindowList
{

void start(HWND hwnd, UINT pollMillis, void (*newWindowCallback)(HWND));
void stop();

} // namespace WindowList
