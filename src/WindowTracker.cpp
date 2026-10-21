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
#include "VirtualDesktop.h"
#include "WindowIcon.h"
#include "WindowInfo.h"
#include "WindowMessage.h"

// Standard library
#include <cassert>
#include <iterator>
#include <list>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

using Items = std::list<WindowTracker::Item>;
using ItemIndex = std::unordered_map<HWND, Items::iterator>;

HWND messageHwnd_;
UINT_PTR timer_;
bool debounceTimerActive_ {};

constexpr UINT_PTR backstopTimerId_ = 1;
constexpr UINT_PTR debounceTimerId_ = 2;
constexpr UINT scanDebounceMillis_ = 200;
constexpr UINT scanBackstopMillis_ = 30000;
// Allow the system to coalesce the backstop timer with other timers.
constexpr ULONG timerToleranceMillis_ = 50;

Items items_;
ItemIndex itemIndex_;
bool enumerating_;

Items::iterator findWindow(HWND hwnd);
Items::iterator addItem(HWND hwnd);
void updateItemTitle(WindowTracker::Item & item);
void reconcileItems();
VOID timerProc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
BOOL CALLBACK enumVisibleWindowsProc(HWND hwnd, LPARAM lParam);
void restoreRemovedVirtualDesktopWindows();

} // anonymous namespace

namespace WindowTracker
{

bool start(HWND messageHwnd)
{
    DEBUG_PRINTF("WindowTracker starting\n");

    messageHwnd_ = messageHwnd;

    DEBUG_PRINTF("WindowTracker setting backstop timer to %u ms\n", scanBackstopMillis_);
    timer_ = SetCoalescableTimer(messageHwnd_, backstopTimerId_, scanBackstopMillis_, timerProc, timerToleranceMillis_);
    if (!timer_) {
        ERROR_PRINTF("SetCoalescableTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    return true;
}

void stop() noexcept
{
    DEBUG_PRINTF("WindowTracker stopping\n");

    assert(!enumerating_);
    items_.clear();
    itemIndex_.clear();

    // The message window may already have been destroyed during shutdown, in
    // which case its timers were destroyed with it.
    if (messageHwnd_ && IsWindow(messageHwnd_)) {
        if (debounceTimerActive_) {
            if (!KillTimer(messageHwnd_, debounceTimerId_)) {
                ERROR_PRINTF("KillTimer() failed: %ld\n", GetLastError());
            }
        }

        if (timer_) {
            if (!KillTimer(messageHwnd_, timer_)) {
                ERROR_PRINTF("KillTimer() failed: %ld\n", GetLastError());
            }
        }
    }

    debounceTimerActive_ = false;
    timer_ = 0;

    messageHwnd_ = nullptr;
}

void requestScan() noexcept
{
    if (!messageHwnd_ || debounceTimerActive_) {
        return;
    }

    debounceTimerActive_ = true;
    if (!SetTimer(messageHwnd_, debounceTimerId_, scanDebounceMillis_, timerProc)) {
        debounceTimerActive_ = false;
        WARNING_PRINTF("could not request window scan, SetTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
    }
}

void updateTitle(HWND hwnd)
{
    assert(!enumerating_);

    const Items::iterator it = findWindow(hwnd);
    if ((it == items_.end()) || !it->trayIcon_) {
        return;
    }

    updateItemTitle(*it);
}

void minimize(HWND hwnd, MinimizePlacement minimizePlacement, MinimizePersistence minimizePersistence)
{
    DEBUG_PRINTF("tray window minimize %#x - '%s'\n", hwnd, WindowInfo::getTitle(hwnd).c_str());

    assert(!enumerating_);

    // If the user removed the hidden virtual desktop, restore any windows that
    // were on it before minimizing this one
    restoreRemovedVirtualDesktopWindows();

    Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        // A window can be shown and minimized before it is tracked, so start
        // tracking it on demand.
        const bool visible = isWindowUserVisible(hwnd);
        if (!visible) {
            DEBUG_PRINTF("not minimizing unknown window %#x\n", hwnd);
            return;
        }
        DEBUG_PRINTF("tracking window %#x on demand\n", hwnd);
        it = addItem(hwnd);
    }

    Item & item = *it;

    if (item.minimized_) {
        DEBUG_PRINTF("not minimizing already minimized window %#x\n", hwnd);
        return;
    }

    if (isUwpWindow(hwnd)) {
        // UWP app windows (ApplicationFrameWindow) do not stay hidden when hidden with ShowWindow,
        // so "minimize" them by moving them to a hidden virtual desktop instead
        if (!VirtualDesktop::minimize(hwnd)) {
            ERROR_PRINTF("failed to minimize UWP window %#x to hidden virtual desktop\n", hwnd);
        }
    } else {
        // minimize and hide window
        // return values intentionally ignored, ShowWindow returns previous visibility
        if (!IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        ShowWindow(hwnd, SW_HIDE);

        if (isWindowUserVisible(hwnd)) {
            ERROR_PRINTF("window is not visible after minimize: %#x\n", hwnd);
        }
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
    itemIndex_[hwnd] = std::prev(items_.end());
}

void restore(HWND hwnd)
{
    DEBUG_PRINTF("tray window restore %#x - '%s'\n", hwnd, WindowInfo::getTitle(hwnd).c_str());

    assert(!enumerating_);

    const Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        WARNING_PRINTF("unknown window restored %#x\n", hwnd);

        // show and restore window
        // return value intentionally ignored, ShowWindow returns previous visibility
        ShowWindow(hwnd, SW_SHOW);
        ShowWindow(hwnd, SW_RESTORE);

        // make window foreground
        // return value intentionally ignored, SetForegroundWindow returns whether brought to foreground
        SetForegroundWindow(hwnd);
        return;
    }

    Item & item = *it;

    if (isUwpWindow(hwnd)) {
        // move UWP app window back from the hidden virtual desktop
        VirtualDesktop::restore(hwnd);
        if (IsIconic(hwnd)) {
            // return value intentionally ignored, ShowWindow returns previous visibility
            ShowWindow(hwnd, SW_SHOW);
            ShowWindow(hwnd, SW_RESTORE);
        }
    } else {
        // show and restore window
        // return value intentionally ignored, ShowWindow returns previous visibility
        ShowWindow(hwnd, SW_SHOW);
        ShowWindow(hwnd, SW_RESTORE);

        if (!isWindowUserVisible(hwnd)) {
            ERROR_PRINTF("window is not visible after restore: %#x\n", hwnd);
        }
    }

    // make window foreground
    // return value intentionally ignored, SetForegroundWindow returns whether brought to foreground
    SetForegroundWindow(hwnd);

    item.minimized_ = false;
    item.visible_ = true;
    if (item.minimizePersistence_ == MinimizePersistence::Never) {
        itemIndex_.erase(hwnd);
        items_.erase(it);
        return;
    }

    // put the item at the front of the list so the next restore is in reverse order of minimize
    items_.push_front(item);
    items_.erase(it);
    itemIndex_[hwnd] = items_.begin();
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

bool isTracked(HWND hwnd)
{
    assert(!enumerating_);

    return findWindow(hwnd) != items_.end();
}

void enumerate(const std::function<bool(const Item &)> & callback)
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

void reverseEnumerate(const std::function<bool(const Item &)> & callback)
{
    enumerating_ = true;

    for (const WindowTracker::Item & item : std::ranges::reverse_view(items_)) {
        // cppcheck-suppress useStlAlgorithm
        if (!callback(item)) {
            break;
        }
    }

    enumerating_ = false;
}

void enumerateVisibleWindows(const std::function<bool(HWND)> & callback)
{
    std::vector<HWND> windows;
    if (!EnumWindows(enumVisibleWindowsProc, reinterpret_cast<LPARAM>(&windows))) {
        ERROR_PRINTF("could not list windows: EnumWindows() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    for (HWND hwnd : windows) {
        if (!callback(hwnd)) {
            break;
        }
    }
}

} // namespace WindowTracker

namespace
{

Items::iterator findWindow(HWND hwnd)
{
    assert(!enumerating_);

    const ItemIndex::iterator it = itemIndex_.find(hwnd);
    if (it == itemIndex_.end()) {
        return items_.end();
    }

    return it->second;
}

Items::iterator addItem(HWND hwnd)
{
    const std::string title = WindowInfo::getTitle(hwnd);
    DEBUG_PRINTF("window added %#x - '%s'\n", hwnd, title.c_str());

    assert(!enumerating_);

    WindowTracker::Item item;
    item.hwnd_ = hwnd;
    item.title_ = title;
    item.visible_ = true;
    items_.push_back(item);

    const Items::iterator it = std::prev(items_.end());
    itemIndex_[hwnd] = it;
    return it;
}

void restoreRemovedVirtualDesktopWindows()
{
    assert(!enumerating_);

    const std::vector<HWND> affected = VirtualDesktop::checkHiddenDesktopRemoved();
    if (affected.empty()) {
        return;
    }

    DEBUG_PRINTF("restoring %zu window(s) from removed hidden virtual desktop\n", affected.size());
    for (HWND hwnd : affected) {
        const Items::iterator it = findWindow(hwnd);
        if (it == items_.end()) {
            continue;
        }

        WindowTracker::Item & item = *it;
        item.minimized_ = false;
        item.visible_ = true;
        if (item.minimizePersistence_ == MinimizePersistence::Never) {
            itemIndex_.erase(hwnd);
            items_.erase(it);
            continue;
        }

        // put the item at the front of the list so the next restore is in reverse order of minimize
        items_.push_front(item);
        items_.erase(it);
        itemIndex_[hwnd] = items_.begin();
    }
}

void updateItemTitle(WindowTracker::Item & item)
{
    assert(item.trayIcon_);

    const std::string title = WindowInfo::getTitle(item.hwnd_);
    if (item.title_ != title) {
        DEBUG_PRINTF("\tchanged window %#x title: to %s\n", item.hwnd_, title.c_str());
        item.title_ = title;
        item.trayIcon_->updateTip(item.title_);
    }
}

void reconcileItems()
{
    assert(!enumerating_);

    for (Items::iterator it = items_.begin(); it != items_.end();) {
        WindowTracker::Item & item = *it;

        if (!IsWindow(item.hwnd_)) {
            DEBUG_PRINTF("\tremoved destroyed window %#x\n", item.hwnd_);
            itemIndex_.erase(item.hwnd_);
            it = items_.erase(it);
            continue;
        }

        const bool visible = isWindowUserVisible(item.hwnd_);
        if (item.minimized_) {
            if (visible) {
                // the window was restored without going through Finestray
                DEBUG_PRINTF("\twindow %#x was restored externally\n", item.hwnd_);
                if (item.minimizePersistence_ == MinimizePersistence::Never) {
                    itemIndex_.erase(item.hwnd_);
                    it = items_.erase(it);
                    continue;
                }
                item.minimized_ = false;
                item.visible_ = true;
            } else {
                item.visible_ = false;
            }
        } else if (!visible) {
            // a persistent tray icon whose window is gone or hidden
            DEBUG_PRINTF("\tremoved hidden window %#x\n", item.hwnd_);
            itemIndex_.erase(item.hwnd_);
            it = items_.erase(it);
            continue;
        }

        if (item.trayIcon_) {
            updateItemTitle(item);
        }

        ++it;
    }
}

VOID timerProc(HWND /* unnamedParam1 */, UINT /* unnamedParam2 */, UINT_PTR timerId, DWORD /* unnamedParam4 */)
{
    const bool backstop = (timerId == backstopTimerId_);

    if (timerId == debounceTimerId_) {
        debounceTimerActive_ = false;
        if (!KillTimer(messageHwnd_, debounceTimerId_)) {
            WARNING_PRINTF("KillTimer() failed: %ld\n", GetLastError());
        }
    } else if (backstop && debounceTimerActive_) {
        // The backstop scan will cover any pending debounce.
        if (!KillTimer(messageHwnd_, debounceTimerId_)) {
            WARNING_PRINTF("KillTimer() failed: %ld\n", GetLastError());
        }
        debounceTimerActive_ = false;
    }

    if (items_.empty()) {
        return;
    }

    // restore any windows that were on the hidden virtual desktop if the user removed it
    restoreRemovedVirtualDesktopWindows();

    reconcileItems();
}

BOOL CALLBACK enumVisibleWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!isWindowUserVisible(hwnd) || !VirtualDesktop::isWindowOnCurrentDesktop(hwnd)) {
        return TRUE;
    }

    std::vector<HWND> & windows = *reinterpret_cast<std::vector<HWND> *>(lParam);
    windows.push_back(hwnd);
    return TRUE;
}

} // anonymous namespace
