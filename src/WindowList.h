// Copyright 2020 Benbuck Nason

#pragma once

// Windows
#include <Windows.h>

namespace WindowList
{

void start(HWND hwnd, UINT pollMillis, void (*addWindowCallback)(HWND), void (*removeWindowCallback)(HWND));
void stop();

} // namespace WindowList
