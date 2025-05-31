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
#include "Log.h"
#include "StringUtility.h"
#include "TrayIcon.h"
#include "WindowIcon.h"
#include "WindowMessage.h"

// Standard library
#include <cassert>
#include <list>
#include <memory>
#include <ranges>
#include <string>

namespace
{

typedef std::list<WindowTracker::Item> Items;

HWND messageHwnd_;
Items items_;
bool enumerating_;

Items::iterator findWindow(HWND hwnd);

} // anonymous namespace

namespace WindowTracker
{

void start(HWND messageHwnd) noexcept
{
    DEBUG_PRINTF("WindowTracker starting\n");

    messageHwnd_ = messageHwnd;
}

void stop() noexcept
{
    DEBUG_PRINTF("WindowTracker stopping\n");

    assert(!enumerating_);
    items_.clear();

    messageHwnd_ = nullptr;
}

bool windowAdded(HWND hwnd)
{
    if (findWindow(hwnd) != items_.end()) {
        WARNING_PRINTF("window already tracked: %#x\n", hwnd);
        return false;
    }

    assert(!enumerating_);

    Item item;
    item.hwnd_ = hwnd;
    item.title_ = getWindowText(hwnd);
    item.visible_ = isWindowUserVisible(hwnd);
    items_.push_back(item);

    DEBUG_PRINTF("window added: %zu items\n", items_.size());
    return true;
}

void windowDestroyed(HWND hwnd)
{
    assert(!enumerating_);

    const Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        WARNING_PRINTF("window not tracked: %#x\n", hwnd);
        return;
    }

    items_.erase(it);

    DEBUG_PRINTF("window destroyed: %zu items\n", items_.size());
}

void windowChanged(HWND hwnd)
{
    assert(!enumerating_);

    const Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        WARNING_PRINTF("window not tracked: %#x\n", hwnd);
        return;
    }

    Item & item = *it;

    const bool visible = isWindowUserVisible(hwnd);
    if (item.visible_ != visible) {
        DEBUG_PRINTF("changed window %#x visibility: to %s\n", hwnd, StringUtility::boolToCString(visible));
        item.visible_ = visible;
    }

    const std::string title = getWindowText(hwnd);
    if (item.title_ != title) {
        DEBUG_PRINTF("changed window %#x title: to %s\n", hwnd, title.c_str());
        item.title_ = title;
        if (item.trayIcon_) {
            item.trayIcon_->updateTip(item.title_);
        }
    }
}

void minimize(HWND hwnd, MinimizePlacement minimizePlacement, MinimizePersistence minimizePersistence)
{
    DEBUG_PRINTF("tray window minimize %#x\n", hwnd);

    assert(!enumerating_);

    const Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        DEBUG_PRINTF("not minimizing unknown window %#x\n", hwnd);
        return;
    }

    Item & item = *it;

    if (item.minimized_) {
        DEBUG_PRINTF("not minimizing already minimized window %#x\n", hwnd);
        return;
    }

    // minimize and hide window
    // return value intentionally ignored, ShowWindow returns previous visibility
    ShowWindow(hwnd, SW_MINIMIZE);
    ShowWindow(hwnd, SW_HIDE);

    if (isWindowUserVisible(hwnd)) {
        ERROR_PRINTF("window is not visible after minimize: %#x\n", hwnd);
    }

    item.minimized_ = true;
    item.visible_ = false;

    // "none" means keep existing persistence
    if (minimizePersistence != MinimizePersistence::None) {
        assert(
            (item.minimizePersistence_ == MinimizePersistence::Never) ||
            (minimizePersistence == MinimizePersistence::Always));
        item.minimizePersistence_ = minimizePersistence;
    }
    assert(item.minimizePersistence_ != MinimizePersistence::None);

    // FIX - persistent implies tray placement

    if (!minimizePlacementIncludesTray(minimizePlacement)) {
        item.trayIcon_.reset();
    } else {
        if (!item.trayIcon_) {
            item.trayIcon_ = std::make_unique<TrayIcon>();
            IconHandleWrapper icon = WindowIcon::get(hwnd);
            const ErrorContext err = item.trayIcon_->create(hwnd, messageHwnd_, WM_TRAYWINDOW, std::move(icon));
            if (err) {
                WARNING_PRINTF("failed to create tray icon for minimized window %#x\n", hwnd);
                errorMessage(err);
                item.trayIcon_.reset();
            }
        }
    }

    // move item to end of list so restore order is reverse of minimize order
    items_.push_back(item);
    items_.erase(it);
}

void restore(HWND hwnd)
{
    DEBUG_PRINTF("tray window restore %#x\n", hwnd);

    assert(!enumerating_);

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

    Item & item = *it;

    item.minimized_ = false;
    item.visible_ = true;
    if (item.minimizePersistence_ == MinimizePersistence::Never) {
        item.trayIcon_.reset();
    }

    // move item to front of list so next restore is in reverse order of minimize
    items_.push_front(item);
    items_.erase(it);
}

void addAllMinimizedToTray(MinimizePlacement minimizePlacement)
{
    assert(!enumerating_);

    for (Item & item : items_) {
        if (!item.minimized_) {
            continue;
        }

        if (minimizePlacementIncludesTray(minimizePlacement)) {
            if (!item.trayIcon_) {
                item.trayIcon_ = std::make_unique<TrayIcon>();
                IconHandleWrapper icon = WindowIcon::get(item.hwnd_);
                const ErrorContext err = item.trayIcon_->create(item.hwnd_, messageHwnd_, WM_TRAYWINDOW, std::move(icon));
                if (err) {
                    WARNING_PRINTF("failed to create tray icon for minimized window %#x\n", item.hwnd_);
                    item.trayIcon_.reset();
                    errorMessage(err);
                }
            }
        } else {
            if (item.trayIcon_) {
                if (item.minimizePersistence_ == MinimizePersistence::Never) {
                    item.trayIcon_.reset();
                }
            }
        }
    }
}

void updateMinimizePlacement(MinimizePlacement minimizePlacement)
{
    if (minimizePlacementIncludesTray(minimizePlacement)) {
        addAllMinimizedToTray(minimizePlacement);
    } else {
        assert(!enumerating_);

        for (Item & item : items_) {
            if (item.minimizePersistence_ == MinimizePersistence::Never) {
                item.trayIcon_.reset();
            }
        }
    }
}

bool isMinimized(HWND hwnd)
{
    assert(!enumerating_);

    const Items::const_iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        return false;
    }

    return it->minimized_;
}

void enumerate(std::function<bool(const Item &)> callback)
{
    enumerating_ = true;

    for (const Item & item : items_) {
        // cppcheck-suppress useStlAlgorithm
        if (!callback(item)) {
            break;
        }
    }

    enumerating_ = false;
}

void reverseEnumerate(std::function<bool(const Item &)> callback)
{
    enumerating_ = true;

    for (Items::reverse_iterator it = items_.rbegin(); it != items_.rend(); ++it) {
        if (!callback(*it)) {
            break;
        }
    }

    enumerating_ = false;
}

} // namespace WindowTracker

namespace
{

Items::iterator findWindow(HWND hwnd)
{
    assert(!enumerating_);

    return std::ranges::find_if(items_, [hwnd](const WindowTracker::Item & item) {
        return item.hwnd_ == hwnd;
    });
}

} // anonymous namespace
