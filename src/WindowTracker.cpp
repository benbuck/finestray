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

// App
#include "WindowTracker.h"
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
#include <ranges>

using WindowTracker::Item;
using WindowTracker::Items;

namespace
{

HWND hwnd_;
UINT pollMillis_;
void (*addWindowCallback_)(HWND);
UINT_PTR timer_;
Items items_;

Items::iterator findWindow(HWND hwnd);
Items::const_iterator findMinimizedWindow(HWND hwnd);
VOID timerProc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);

} // anonymous namespace

namespace WindowTracker
{

void start(HWND hwnd, UINT pollMillis, void (*addWindowCallback)(HWND))
{
    DEBUG_PRINTF("WindowTracker starting\n");

    hwnd_ = hwnd;
    pollMillis_ = pollMillis;
    addWindowCallback_ = addWindowCallback;

    if (pollMillis_ > 0) {
        DEBUG_PRINTF("WindowTracker setting poll timer to %u\n", pollMillis_);
        timer_ = SetTimer(hwnd_, 1, pollMillis_, timerProc);
        if (!timer_) {
            ERROR_PRINTF("SetTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

void stop()
{
    DEBUG_PRINTF("WindowTracker stopping\n");

    items_.clear();

    if (timer_) {
        if (!KillTimer(hwnd_, timer_)) {
            ERROR_PRINTF("KillTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        timer_ = 0;
    }

    addWindowCallback_ = nullptr;
    pollMillis_ = 0;
    hwnd_ = nullptr;
}

const Items & getAll()
{
    return items_;
}

HWND getVisibleIndex(unsigned int index)
{
    if (index >= items_.size()) {
        return nullptr;
    }

    unsigned int count = 0;
    for (const auto & item : items_) {
        if (item.visible_) {
            if (count == index) {
                return item.hwnd_;
            }
            ++count;
        }
    }

    return nullptr;
}

void minimize(HWND hwnd, MinimizePlacement minimizePlacement)
{
    DEBUG_PRINTF("tray window minimize %#x\n", hwnd);

    const Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        DEBUG_PRINTF("not minimizing unknown window %#x\n", hwnd);
        return;
    }

    if (it->minimized_) {
        DEBUG_PRINTF("not minimizing already minimized window %#x\n", hwnd);
        return;
    }

    // minimize and hide window
    // return value intentionally ignored, ShowWindow returns previous visibility
    ShowWindow(hwnd, SW_MINIMIZE);
    ShowWindow(hwnd, SW_HIDE);

    std::unique_ptr<TrayIcon> trayIcon;
    if (minimizePlacementIncludesTray(minimizePlacement)) {
        trayIcon = std::make_unique<TrayIcon>();
        IconHandleWrapper icon = WindowIcon::get(hwnd);
        const ErrorContext err = trayIcon->create(hwnd, hwnd_, WM_TRAYWINDOW, std::move(icon));
        if (err) {
            WARNING_PRINTF("failed to create tray icon for minimized window %#x\n", hwnd);
            errorMessage(err);
            return;
        }
    }

    it->minimized_ = true;
    it->trayIcon_ = std::move(trayIcon);
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

    if (!isWindowUserVisible(hwnd)) {
        ERROR_PRINTF("window is not visible after restore: %#x\n", hwnd);
    }

    const Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        WARNING_PRINTF("unknown window restored %#x\n", hwnd);
        return;
    }

    it->minimized_ = false;
    it->trayIcon_.reset();
}

void addAllMinimizedToTray(MinimizePlacement minimizePlacement)
{
    for (Item & item : items_) {
        if (!item.minimized_) {
            continue;
        }

        if (minimizePlacementIncludesTray(minimizePlacement)) {
            if (!item.trayIcon_) {
                item.trayIcon_ = std::make_unique<TrayIcon>();
                IconHandleWrapper icon = WindowIcon::get(item.hwnd_);
                const ErrorContext err = item.trayIcon_->create(item.hwnd_, hwnd_, WM_TRAYWINDOW, std::move(icon));
                if (err) {
                    WARNING_PRINTF("failed to create tray icon for minimized window %#x\n", item.hwnd_);
                    item.trayIcon_.reset();
                    errorMessage(err);
                }
            }
        } else {
            if (item.trayIcon_) {
                item.trayIcon_.reset();
            }
        }
    }
}

void updateMinimizePlacement(MinimizePlacement minimizePlacement)
{
    if (minimizePlacementIncludesTray(minimizePlacement)) {
        addAllMinimizedToTray(minimizePlacement);
    } else {
        for (Item & item : items_) {
            item.trayIcon_.reset();
        }
    }
}

HWND getFromID(UINT id)
{
    const Items::const_iterator it = std::ranges::find_if(items_, [id](const Item & item) noexcept {
        return item.trayIcon_ && (item.trayIcon_->id() == id);
    });
    if (it == items_.end()) {
        return nullptr;
    }

    return it->hwnd_;
}

HWND getMinimizedIndex(unsigned int index)
{
    if (index >= items_.size()) {
        return nullptr;
    }

    unsigned int count = 0;
    for (const auto & item : items_) {
        if (item.minimized_) {
            if (count == index) {
                return item.hwnd_;
            }
            ++count;
        }
    }

    return nullptr;
}

HWND getLastMinimized() noexcept
{
    for (Items::reverse_iterator it = items_.rbegin(); it != items_.rend(); ++it) {
        if (it->minimized_) {
            return it->hwnd_;
        }
    }

    return nullptr;
}

std::vector<HWND> getAllMinimized()
{
    std::vector<HWND> minimizedWindows;

    for (const Item & item : items_) {
        if (item.minimized_) {
            if (item.visible_) {
                // window is visible, but minimized
                DEBUG_PRINTF("window is visible and minimized: %#x\n", item.hwnd_);
                __debugbreak();
            }
            minimizedWindows.push_back(item.hwnd_);
        }
    }

    return minimizedWindows;
}

std::vector<HWND> getAllVisible()
{
    std::vector<HWND> visibleWindows;

    for (const Item & item : items_) {
        if (item.visible_ && !item.minimized_) {
            visibleWindows.push_back(item.hwnd_);
        }
    }

    return visibleWindows;
}

bool isMinimized(HWND hwnd)
{
    return findMinimizedWindow(hwnd) != items_.end();
}

} // namespace WindowTracker

namespace
{

Items::iterator findWindow(HWND hwnd)
{
    return std::ranges::find_if(items_, [hwnd](const Item & item) {
        return item.hwnd_ == hwnd;
    });
}

Items::const_iterator findMinimizedWindow(HWND hwnd)
{
    return std::ranges::find_if(items_, [hwnd](const Item & item) {
        return (item.hwnd_ == hwnd) && item.minimized_;
    });
}

VOID timerProc(
    HWND /* unnamedParam1 */,
    UINT /* unnamedParam2 */,
    UINT_PTR /* unnamedParam3 */,
    DWORD /* unnamedParam4 */)
{
    std::vector<HWND> newWindows;
    if (!EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&newWindows))) {
        ERROR_PRINTF("could not list windows: EnumWindows() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    // check for removed windows
    for (Items::iterator it = items_.begin(); it != items_.end();) {
        std::vector<HWND>::const_iterator nit = std::find(newWindows.begin(), newWindows.end(), it->hwnd_);
        if (nit != newWindows.end()) {
            ++it;
        } else {
            it = items_.erase(it);
        }
    }

    // check for added windows
    for (HWND hwnd : newWindows) {
        Items::iterator it = findWindow(hwnd);
        if (it == items_.end()) {
            Item item;
            item.hwnd_ = hwnd;
            item.title_ = getWindowText(hwnd);
            item.visible_ = isWindowUserVisible(hwnd);
            items_.push_back(item);

            if (addWindowCallback_) {
                addWindowCallback_(hwnd);
            }
        }
    }

    if (items_.size() != newWindows.size()) {
        __debugbreak();
    }

    // check for changed window titles or visibility
    for (auto & item : items_) {
        // existing window found

        // window title changed
        const std::string title = getWindowText(item.hwnd_);
        if (item.title_ != title) {
            DEBUG_PRINTF("changed window title: %#x to %s\n", item.hwnd_, title.c_str());
            item.title_ = title;
            item.trayIcon_->updateTip(item.title_);
        }

        // window visibility changed
        bool visible = isWindowUserVisible(item.hwnd_);
        if (item.visible_ != visible) {
            DEBUG_PRINTF("changed window visibility: %#x to %s\n", item.hwnd_, StringUtility::boolToCString(visible));
            item.visible_ = visible;
        }
    }
}

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    // ignore windows that can't be visible to the user
    if (isWindowStealth(hwnd)) {
        return TRUE;
    }

    std::vector<HWND> & windows = *reinterpret_cast<std::vector<HWND> *>(lParam);
    windows.push_back(hwnd);
    return TRUE;
}

} // anonymous namespace
