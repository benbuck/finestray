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
#include "MinimizePersistence.h"
#include "MinimizePlacement.h"

// Windows
#include <Windows.h>

// Standard library
#include <functional>
#include <memory>
#include <string>
#include <vector>

class TrayIcon;

namespace WindowTracker
{
struct Item
{
    HWND hwnd_ {};
    std::string title_;
    bool visible_ {};
    bool minimized_ {};
    MinimizePersistence minimizePersistence_ { MinimizePersistence::Never };
    std::shared_ptr<TrayIcon> trayIcon_;
};

void start(HWND messageHwnd) noexcept;
void stop() noexcept;
bool windowAdded(HWND hwnd);
void windowDestroyed(HWND hwnd);
void windowChanged(HWND hwnd);
void minimize(HWND hwnd, MinimizePlacement minimizePlacement, MinimizePersistence minimizePersistence);
void restore(HWND hwnd);
void addAllMinimizedToTray(MinimizePlacement minimizePlacement);
void updateMinimizePlacement(MinimizePlacement minimizePlacement);
bool isMinimized(HWND hwnd);
void enumerate(std::function<bool(const Item &)> callback);
void reverseEnumerate(std::function<bool(const Item &)> callback);

} // namespace WindowTracker
