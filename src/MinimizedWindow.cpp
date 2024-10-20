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

#include "MinimizedWindow.h"

// App
#include "DebugPrint.h"
#include "StringUtility.h"
#include "TrayIcon.h"
#include "WindowIcon.h"
#include "WindowMessage.h"

// Standard library
#include <list>
#include <memory>

namespace MinimizedWindow
{

struct MinimizedWindowData
{
    MinimizedWindowData(HWND hwnd, HWND messageHwnd, std::unique_ptr<TrayIcon> && trayIcon)
        : hwnd_(hwnd)
        , messageHwnd_(messageHwnd)
        , trayIcon_(std::move(trayIcon))
    {
    }

    HWND hwnd_;
    HWND messageHwnd_;
    std::unique_ptr<TrayIcon> trayIcon_;
};

typedef std::vector<MinimizedWindowData> MinimizedWindows;

static MinimizedWindows minimizedWindows_;

static MinimizedWindows::iterator findMinimizedWindow(HWND hwnd);
static TrayIcon * createTrayIcon(HWND hwnd, HWND messageHwnd);

void minimize(HWND hwnd, HWND messageHwnd, MinimizePlacement minimizePlacement)
{
    DEBUG_PRINTF("tray window minimize %#x\n", hwnd);

    MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it != minimizedWindows_.end()) {
        DEBUG_PRINTF("not minimizing already minimized window %#x\n", hwnd);
        return;
    }

    // minimize window
    if (!ShowWindow(hwnd, SW_MINIMIZE)) {
        DEBUG_PRINTF(
            "failed to minimize window %#x, ShowWindow() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    // hide window
    if (!ShowWindow(hwnd, SW_HIDE)) {
        DEBUG_PRINTF("failed to hide window %#x, ShowWindow() failed: %s\n", hwnd, StringUtility::lastErrorString().c_str());
    }

    std::unique_ptr<TrayIcon> trayIcon;
    if (minimizePlacementIncludesTray(minimizePlacement)) {
        trayIcon.reset(createTrayIcon(hwnd, messageHwnd));
    }

    // un-hide window on failure, but leave it minimized
    // DEBUG_PRINTF("failed to add tray icon for %#x\n", hwnd);
    // if (!ShowWindow(hwnd, SW_SHOW)) {
    //    DEBUG_PRINTF("failed to show window %#x, ShowWindow() failed: %s\n", hwnd,
    //    StringUtility::lastErrorString().c_str());
    // }

    minimizedWindows_.emplace_back(hwnd, messageHwnd, std::move(trayIcon));
}

void restore(HWND hwnd)
{
    DEBUG_PRINTF("tray window restore %#x\n", hwnd);

    if (!ShowWindow(hwnd, SW_SHOW)) {
        DEBUG_PRINTF("failed to show window %#x, ShowWindow() failed: %s\n", hwnd, StringUtility::lastErrorString().c_str());
    }

    if (!ShowWindow(hwnd, SW_RESTORE)) {
        DEBUG_PRINTF(
            "failed to restore window %#x, ShowWindow() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    if (!SetForegroundWindow(hwnd)) {
        DEBUG_PRINTF(
            "failed to set foreground window %#x, SetForegroundWindow() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it == minimizedWindows_.end()) {
        DEBUG_PRINTF("failed to restore minimized window %#x, not found\n", hwnd);
    } else {
        minimizedWindows_.erase(it);
    }
}

void addAll(MinimizePlacement minimizePlacement)
{
    for (MinimizedWindows::iterator it = minimizedWindows_.begin(); it != minimizedWindows_.end(); ++it) {
        MinimizedWindowData & minimizedWindow = *it;
        if (minimizePlacementIncludesTray(minimizePlacement)) {
            if (!minimizedWindow.trayIcon_) {
                minimizedWindow.trayIcon_.reset(createTrayIcon(minimizedWindow.hwnd_, minimizedWindow.messageHwnd_));
            }
        } else {
            if (minimizedWindow.trayIcon_) {
                minimizedWindow.trayIcon_.reset();
            }
        }
    }
}

void restoreAll()
{
    while (!minimizedWindows_.empty()) {
        const MinimizedWindowData & minimizedWindow = minimizedWindows_.back();
        restore(minimizedWindow.hwnd_);
    }
}

void updatePlacement(MinimizePlacement minimizePlacement)
{
    if (minimizePlacementIncludesTray(minimizePlacement)) {
        addAll(minimizePlacement);
    } else {
        for (MinimizedWindows::iterator it = minimizedWindows_.begin(); it != minimizedWindows_.end(); ++it) {
            MinimizedWindowData & minimizedWindow = *it;
            minimizedWindow.trayIcon_.reset();
        }
    }
}

HWND getFromID(UINT id)
{
    for (MinimizedWindows::const_iterator cit = minimizedWindows_.cbegin(); cit != minimizedWindows_.cend(); ++cit) {
        const MinimizedWindowData & minimizedWindow = *cit;
        if (minimizedWindow.trayIcon_ && (minimizedWindow.trayIcon_->id() == id)) {
            return minimizedWindow.hwnd_;
        }
    }

    return nullptr;
}

HWND getLast()
{
    if (minimizedWindows_.empty()) {
        return nullptr;
    }

    return minimizedWindows_.back().hwnd_;
}

std::vector<HWND> getAll()
{
    std::vector<HWND> minimizedWindows;
    for (MinimizedWindows::const_iterator cit = minimizedWindows_.cbegin(); cit != minimizedWindows_.cend(); ++cit) {
        const MinimizedWindowData & minimizedWindow = *cit;
        minimizedWindows.push_back(minimizedWindow.hwnd_);
    }
    return minimizedWindows;
}

MinimizedWindows::iterator findMinimizedWindow(HWND hwnd)
{
    return std::find_if(
        minimizedWindows_.begin(),
        minimizedWindows_.end(),
        [hwnd](const MinimizedWindowData & minimizedWindow) {
            return minimizedWindow.hwnd_ == hwnd;
        });
}

bool exists(HWND hwnd)
{
    return findMinimizedWindow(hwnd) != minimizedWindows_.end();
}

TrayIcon * createTrayIcon(HWND hwnd, HWND messageHwnd)
{
    TrayIcon * trayIcon = new TrayIcon();

    HICON hicon = WindowIcon::get(hwnd);
    if (!hicon) {
        hicon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    trayIcon->create(hwnd, messageHwnd, WM_TRAYWINDOW, hicon);

    return trayIcon;
}

} // namespace MinimizedWindow
