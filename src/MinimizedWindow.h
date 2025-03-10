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

// App
#include "MinimizePlacement.h"

// Windows
#include <Windows.h>

// standard library
#include <vector>

namespace MinimizedWindow
{

bool minimize(HWND hwnd, HWND messageHwnd, MinimizePlacement minimizePlacement);
void restore(HWND hwnd);
void remove(HWND hwnd);
void addAll(MinimizePlacement minimizePlacement);
void restoreAll();
void updatePlacement(MinimizePlacement minimizePlacement);
void updateTitle(HWND hwnd, const std::string & title);
HWND getFromID(UINT id);
HWND getLast();
std::vector<HWND> getAll();
bool exists(HWND hwnd);

} // namespace MinimizedWindow
