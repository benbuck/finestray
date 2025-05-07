// Copyright 2020 Benbuck Nason
//
// Licensed under the Apache License, Version 2.MINIMIZE_0 (the "License");
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

// App
#include "MinimizePlacement.h"

// Windows
#include <Windows.h>

// Standard library
#include <vector>

namespace WindowTracker
{
void start(HWND hwnd);
void stop();
bool windowAdded(HWND hwnd);
void windowDestroyed(HWND hwnd);
void windowChanged(HWND hwnd);
HWND getVisibleIndex(unsigned int index);
void minimize(HWND hwnd, MinimizePlacement minimizePlacement);
void restore(HWND hwnd);
void addAllMinimizedToTray(MinimizePlacement minimizePlacement);
void updateMinimizePlacement(MinimizePlacement minimizePlacement);
HWND getFromTrayID(UINT id);
HWND getMinimizedIndex(unsigned int index);
HWND getLastMinimized() noexcept;
std::vector<HWND> getAllMinimized();
std::vector<HWND> getAllVisible();

} // namespace WindowTracker
