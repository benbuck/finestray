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
#include "WindowInfo.h"
#include "WindowMessage.h"

// Standard library
#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

namespace
{

typedef std::list<WindowTracker::Item> Items;

HWND messageHwnd_;
UINT pollMillis_;
void (*addWindowCallback_)(HWND);
UINT_PTR timer_;
Items items_;
bool enumerating_;

Items::iterator findWindow(HWND hwnd);
void addItem(HWND hwnd);
void updateItem(WindowTracker::Item & item, HWND hwnd);
VOID timerProc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);

} // anonymous namespace

namespace WindowTracker
{

void start(HWND messageHwnd, UINT pollMillis, void (*addWindowCallback)(HWND))
{
    DEBUG_PRINTF("WindowTracker starting\n");

    messageHwnd_ = messageHwnd;
    pollMillis_ = pollMillis;
    addWindowCallback_ = addWindowCallback;

    if (pollMillis_ > 0) {
        DEBUG_PRINTF("WindowTracker setting poll timer to %u\n", pollMillis_);
        timer_ = SetTimer(messageHwnd_, 1, pollMillis_, timerProc);
        if (!timer_) {
            ERROR_PRINTF("SetTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

void stop() noexcept
{
    DEBUG_PRINTF("WindowTracker stopping\n");

    assert(!enumerating_);
    items_.clear();

    if (timer_) {
        if (!KillTimer(messageHwnd_, timer_)) {
            ERROR_PRINTF("KillTimer() failed: %ld\n", GetLastError());
        }
        timer_ = 0;
    }

    addWindowCallback_ = nullptr;
    pollMillis_ = 0;
    messageHwnd_ = nullptr;
}

void minimize(HWND hwnd, MinimizePlacement minimizePlacement, MinimizePersistence minimizePersistence)
{
    DEBUG_PRINTF("tray window minimize %#x - '%s'\n", hwnd, WindowInfo::getTitle(hwnd).c_str());

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
    DEBUG_PRINTF("tray window restore %#x - '%s'\n", hwnd, WindowInfo::getTitle(hwnd).c_str());

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

    // put the item at the front of the list so the next restore is in reverse order of minimize
    items_.push_front(item);
    items_.erase(it);
}

void addAllMinimizedToTray(MinimizePlacement minimizePlacement)
{
    DEBUG_PRINTF(
        "adding all minimized windows to tray with placement '%s'\n",
        minimizePlacementToCString(minimizePlacement));

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
    DEBUG_PRINTF("updating minimize placement to '%s'\n", minimizePlacementToCString(minimizePlacement));

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

void addItem(HWND hwnd)
{
    const std::string title = WindowInfo::getTitle(hwnd);
    const bool visible = isWindowUserVisible(hwnd);
    DEBUG_PRINTF("window added %#x - '%s' (%s)\n", hwnd, title.c_str(), visible ? "visible" : "invisible");

    assert(!enumerating_);

    WindowTracker::Item item;
    item.hwnd_ = hwnd;
    item.title_ = title;
    item.visible_ = visible;
    items_.push_back(item);
}

void updateItem(WindowTracker::Item & item, HWND hwnd)
{
    const bool visible = isWindowUserVisible(hwnd);
    if (item.visible_ != visible) {
        DEBUG_PRINTF("\tchanged window %#x visibility: to %s\n", hwnd, StringUtility::boolToCString(visible));
        item.visible_ = visible;
    }

    const std::string title = WindowInfo::getTitle(hwnd);
    if (item.title_ != title) {
        DEBUG_PRINTF("\tchanged window %#x title: to %s\n", hwnd, title.c_str());
        item.title_ = title;
        if (item.trayIcon_) {
            item.trayIcon_->updateTip(item.title_);
        }
    }
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
        const std::vector<HWND>::const_iterator nit = std::ranges::find(newWindows, it->hwnd_);
        if (nit != newWindows.end()) {
            ++it;
        } else {
            it = items_.erase(it);
        }
    }

    // check for added windows
    for (HWND hwnd : newWindows) {
        if (findWindow(hwnd) == items_.end()) {
            addItem(hwnd);
            if (addWindowCallback_) {
                addWindowCallback_(hwnd);
            }
        }
    }

    // check for changed window titles or visibility
    for (WindowTracker::Item & item : items_) {
        updateItem(item, item.hwnd_);
    }
}

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    // ignore windows that can't be visible to the user, unless we're tracking
    // them as minimized in the tray
    if (isWindowStealth(hwnd) && !WindowTracker::isMinimized(hwnd)) {
        return TRUE;
    }

    std::vector<HWND> & windows = *reinterpret_cast<std::vector<HWND> *>(lParam);
    windows.push_back(hwnd);
    return TRUE;
}

} // anonymous namespace
