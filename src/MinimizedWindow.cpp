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
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"
#include "TrayIcon.h"
#include "WindowIcon.h"
#include "WindowMessage.h"

// Standard library
#include <list>
#include <memory>

namespace
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

MinimizedWindows minimizedWindows_;

MinimizedWindows::iterator findMinimizedWindow(HWND hwnd);

} // anonymous namespace

namespace MinimizedWindow
{

bool minimize(HWND hwnd, HWND messageHwnd, MinimizePlacement minimizePlacement)
{
    DEBUG_PRINTF("tray window minimize %#x\n", hwnd);

    MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it != minimizedWindows_.end()) {
        DEBUG_PRINTF("not minimizing already minimized window %#x\n", hwnd);
        return false;
    }

    // minimize and hide window
    if (!ShowWindow(hwnd, SW_MINIMIZE)) {
        WARNING_PRINTF(
            "failed to minimize window %#x, ShowWindow() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
        return false;
    } else {
        if (!ShowWindow(hwnd, SW_HIDE)) {
            WARNING_PRINTF(
                "failed to hide window %#x, ShowWindow() failed: %s\n",
                hwnd,
                StringUtility::lastErrorString().c_str());
            return false;
        }
    }

    std::unique_ptr<TrayIcon> trayIcon;
    if (minimizePlacementIncludesTray(minimizePlacement)) {
        trayIcon.reset(new TrayIcon());
        HICON hicon = WindowIcon::get(hwnd);
        ErrorContext err = trayIcon->create(hwnd, messageHwnd, WM_TRAYWINDOW, hicon);
        if (err) {
            WARNING_PRINTF("failed to create tray icon for minimized window %#x\n", hwnd);
            trayIcon.reset();
            errorMessage(err);
            return false;
        }
    }

    minimizedWindows_.emplace_back(hwnd, messageHwnd, std::move(trayIcon));
    return true;
}

void restore(HWND hwnd)
{
    DEBUG_PRINTF("tray window restore %#x\n", hwnd);

    // show and restore window
    if (!ShowWindow(hwnd, SW_SHOWNORMAL)) {
        WARNING_PRINTF(
            "failed to show window %#x, ShowWindow() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    // make window foreground
    if (!SetForegroundWindow(hwnd)) {
        WARNING_PRINTF(
            "failed to set foreground window %#x, SetForegroundWindow() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    remove(hwnd);
}

void remove(HWND hwnd)
{
    MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it == minimizedWindows_.end()) {
        WARNING_PRINTF("failed to remove minimized window %#x, not found\n", hwnd);
        return;
    }

    DEBUG_PRINTF("removing minimized window %#x\n", hwnd);
    minimizedWindows_.erase(it);
}

void addAll(MinimizePlacement minimizePlacement)
{
    for (MinimizedWindows::iterator it = minimizedWindows_.begin(); it != minimizedWindows_.end(); ++it) {
        MinimizedWindowData & minimizedWindow = *it;
        if (minimizePlacementIncludesTray(minimizePlacement)) {
            if (!minimizedWindow.trayIcon_) {
                minimizedWindow.trayIcon_.reset(new TrayIcon());
                HICON hicon = WindowIcon::get(minimizedWindow.hwnd_);
                ErrorContext err = minimizedWindow.trayIcon_->create(
                    minimizedWindow.hwnd_,
                    minimizedWindow.messageHwnd_,
                    WM_TRAYWINDOW,
                    hicon);
                if (err) {
                    WARNING_PRINTF("failed to create tray icon for minimized window %#x\n", minimizedWindow.hwnd_);
                    minimizedWindow.trayIcon_.reset();
                    errorMessage(err);
                }
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

void updateTitle(HWND hwnd, const std::string & title)
{
    MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it == minimizedWindows_.end()) {
        WARNING_PRINTF("failed to update title for minimized window %#x, not found\n", hwnd);
    } else {
        MinimizedWindowData & minimizedWindow = *it;
        minimizedWindow.trayIcon_->updateTip(title);
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

bool exists(HWND hwnd)
{
    return findMinimizedWindow(hwnd) != minimizedWindows_.end();
}

} // namespace MinimizedWindow

namespace
{

MinimizedWindows::iterator findMinimizedWindow(HWND hwnd)
{
    return std::find_if(
        minimizedWindows_.begin(),
        minimizedWindows_.end(),
        [hwnd](const MinimizedWindowData & minimizedWindow) {
            return minimizedWindow.hwnd_ == hwnd;
        });
}

} // anonymous namespace
