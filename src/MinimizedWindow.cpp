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
#include "IconHandleWrapper.h"
#include "Log.h"
#include "StringUtility.h"
#include "TrayIcon.h"
#include "WindowIcon.h"
#include "WindowMessage.h"

// Standard library
#include <algorithm>
#include <iterator>
#include <list>
#include <memory>

namespace
{

struct MinimizedWindowData
{
    HWND hwnd_ {};
    HWND messageHwnd_ {};
    std::unique_ptr<TrayIcon> trayIcon_;
};

using MinimizedWindows = std::vector<MinimizedWindowData>;

MinimizedWindows minimizedWindows_;

MinimizedWindows::iterator findMinimizedWindow(HWND hwnd);

} // anonymous namespace

namespace MinimizedWindow
{

bool minimize(HWND hwnd, HWND messageHwnd, MinimizePlacement minimizePlacement)
{
    DEBUG_PRINTF("tray window minimize %#x\n", hwnd);

    const MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it != minimizedWindows_.end()) {
        DEBUG_PRINTF("not minimizing already minimized window %#x\n", hwnd);
        return false;
    }

    // minimize and hide window
    // return value intentionally ignored, ShowWindow returns previous visibility
    ShowWindow(hwnd, SW_MINIMIZE);
    ShowWindow(hwnd, SW_HIDE);

    std::unique_ptr<TrayIcon> trayIcon;
    if (minimizePlacementIncludesTray(minimizePlacement)) {
        trayIcon = std::make_unique<TrayIcon>();
        IconHandleWrapper icon = WindowIcon::get(hwnd);
        const ErrorContext err = trayIcon->create(hwnd, messageHwnd, WM_TRAYWINDOW, std::move(icon));
        if (err) {
            WARNING_PRINTF("failed to create tray icon for minimized window %#x\n", hwnd);
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
    // return value intentionally ignored, ShowWindow returns previous visibility
    ShowWindow(hwnd, SW_SHOWNORMAL);

    // make window foreground
    // return value intentionally ignored, SetForegroundWindow returns whether brought to foreground
    SetForegroundWindow(hwnd);

    remove(hwnd);
}

void remove(HWND hwnd)
{
    const MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it == minimizedWindows_.end()) {
        WARNING_PRINTF("failed to remove minimized window %#x, not found\n", hwnd);
        return;
    }

    DEBUG_PRINTF("removing minimized window %#x\n", hwnd);
    minimizedWindows_.erase(it);
}

void addAll(MinimizePlacement minimizePlacement)
{
    for (MinimizedWindowData & minimizedWindow : minimizedWindows_) {
        if (minimizePlacementIncludesTray(minimizePlacement)) {
            if (!minimizedWindow.trayIcon_) {
                minimizedWindow.trayIcon_ = std::make_unique<TrayIcon>();
                IconHandleWrapper icon = WindowIcon::get(minimizedWindow.hwnd_);
                const ErrorContext err = minimizedWindow.trayIcon_->create(
                    minimizedWindow.hwnd_,
                    minimizedWindow.messageHwnd_,
                    WM_TRAYWINDOW,
                    std::move(icon));
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
        for (MinimizedWindowData & minimizedWindow : minimizedWindows_) {
            minimizedWindow.trayIcon_.reset();
        }
    }
}

void updateTitle(HWND hwnd, const std::string & title)
{
    const MinimizedWindows::iterator it = findMinimizedWindow(hwnd);
    if (it == minimizedWindows_.end()) {
        WARNING_PRINTF("failed to update title for minimized window %#x, not found\n", hwnd);
    } else {
        const MinimizedWindowData & minimizedWindow = *it;
        minimizedWindow.trayIcon_->updateTip(title);
    }
}

HWND getFromID(UINT id)
{
    const MinimizedWindows::iterator it = std::find_if(
        minimizedWindows_.begin(),
        minimizedWindows_.end(),
        [id](const MinimizedWindowData & minimizedWindow) {
            return minimizedWindow.trayIcon_ && (minimizedWindow.trayIcon_->id() == id);
        });
    if (it == minimizedWindows_.end()) {
        return nullptr;
    }

    return it->hwnd_;
}

HWND getFromIndex(UINT index)
{
    if (index >= minimizedWindows_.size()) {
        return nullptr;
    }

    return minimizedWindows_.at(index).hwnd_;
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
    std::transform(
        minimizedWindows_.begin(),
        minimizedWindows_.end(),
        std::back_inserter(minimizedWindows),
        [](const MinimizedWindowData & minimizedWindow) {
            return minimizedWindow.hwnd_;
        });
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
