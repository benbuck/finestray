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
#include <algorithm>
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

struct TrackedWindow
{
    HWND hwnd_ {};
    bool visible_ {};
};

HWND messageHwnd_;
UINT pollMillis_;
// Allow the system to coalesce the poll timer with other timers.
constexpr ULONG timerToleranceMillis_ = 50;
void (*addWindowCallback_)(HWND);
UINT_PTR timer_;
Items items_;
ItemIndex itemIndex_;
std::vector<TrackedWindow> trackedWindows_;
bool enumerating_;

Items::iterator findWindow(HWND hwnd);
Items::iterator addItem(HWND hwnd, bool visible);
void updateItem(WindowTracker::Item & item, bool visible);
VOID timerProc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);
void restoreRemovedVirtualDesktopWindows();

} // anonymous namespace

namespace WindowTracker
{

bool start(HWND messageHwnd, UINT pollMillis, void (*addWindowCallback)(HWND))
{
    DEBUG_PRINTF("WindowTracker starting\n");

    messageHwnd_ = messageHwnd;
    pollMillis_ = pollMillis;
    addWindowCallback_ = addWindowCallback;

    if (pollMillis_ > 0) {
        DEBUG_PRINTF("WindowTracker setting poll timer to %u\n", pollMillis_);
        timer_ = SetCoalescableTimer(messageHwnd_, 1, pollMillis_, timerProc, timerToleranceMillis_);
        if (!timer_) {
            ERROR_PRINTF("SetCoalescableTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
            return false;
        }
    }

    return true;
}

void stop() noexcept
{
    DEBUG_PRINTF("WindowTracker stopping\n");

    assert(!enumerating_);
    items_.clear();
    itemIndex_.clear();

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

    // If the user removed the hidden virtual desktop, restore any windows that
    // were on it before minimizing this one
    restoreRemovedVirtualDesktopWindows();

    Items::iterator it = findWindow(hwnd);
    if (it == items_.end()) {
        // A window can be shown and minimized faster than the tracking scan
        // runs, so start tracking it on demand.
        const bool visible = isWindowUserVisible(hwnd);
        if (!visible) {
            DEBUG_PRINTF("not minimizing unknown window %#x\n", hwnd);
            return;
        }
        DEBUG_PRINTF("tracking window %#x on demand\n", hwnd);
        it = addItem(hwnd, visible);
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
        item.trayIcon_.reset();
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

Items::iterator addItem(HWND hwnd, bool visible)
{
    const std::string title = WindowInfo::getTitle(hwnd);
    DEBUG_PRINTF("window added %#x - '%s' (%s)\n", hwnd, title.c_str(), visible ? "visible" : "invisible");

    assert(!enumerating_);

    WindowTracker::Item item;
    item.hwnd_ = hwnd;
    item.title_ = title;
    item.visible_ = visible;
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
            item.trayIcon_.reset();
        }

        // put the item at the front of the list so the next restore is in reverse order of minimize
        items_.push_front(item);
        items_.erase(it);
        itemIndex_[hwnd] = items_.begin();
    }
}

void updateItem(WindowTracker::Item & item, bool visible)
{
    if (item.visible_ != visible) {
        DEBUG_PRINTF("\tchanged window %#x visibility: to %s\n", item.hwnd_, StringUtility::boolToCString(visible));
        item.visible_ = visible;
    }

    if (item.trayIcon_) {
        const std::string title = WindowInfo::getTitle(item.hwnd_);
        if (item.title_ != title) {
            DEBUG_PRINTF("\tchanged window %#x title: to %s\n", item.hwnd_, title.c_str());
            item.title_ = title;
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
    trackedWindows_.clear();
    if (!EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&trackedWindows_))) {
        ERROR_PRINTF("could not list windows: EnumWindows() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    // check for removed windows
    for (Items::iterator it = items_.begin(); it != items_.end();) {
        const std::vector<TrackedWindow>::const_iterator nit =
            std::ranges::find_if(trackedWindows_, [hwnd = it->hwnd_](const TrackedWindow & trackedWindow) {
                return trackedWindow.hwnd_ == hwnd;
            });
        if (nit != trackedWindows_.end()) {
            ++it;
        } else {
            itemIndex_.erase(it->hwnd_);
            it = items_.erase(it);
        }
    }

    // check for added windows
    for (const TrackedWindow & trackedWindow : trackedWindows_) {
        if (findWindow(trackedWindow.hwnd_) == items_.end()) {
            addItem(trackedWindow.hwnd_, trackedWindow.visible_);
            if (addWindowCallback_) {
                addWindowCallback_(trackedWindow.hwnd_);
            }
        }
    }

    // check for changed window titles or visibility
    for (const TrackedWindow & trackedWindow : trackedWindows_) {
        const Items::iterator it = findWindow(trackedWindow.hwnd_);
        if (it != items_.end()) {
            updateItem(*it, trackedWindow.visible_);
        }
    }

    // restore any windows that were on the hidden virtual desktop if the user removed it
    restoreRemovedVirtualDesktopWindows();
}

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    // Track windows we minimized to the tray even though they are hidden,
    // otherwise only track windows the user can currently see.
    bool tracked = WindowTracker::isMinimized(hwnd);
    bool visible = false;
    if (tracked) {
        visible = isWindowUserVisible(hwnd);
    } else if (IsWindowVisible(hwnd)) {
        visible = !isWindowStealth(hwnd);
        tracked = visible;
    }

    if (!tracked) {
        return TRUE;
    }

    std::vector<TrackedWindow> & windows = *reinterpret_cast<std::vector<TrackedWindow> *>(lParam);
    windows.push_back(TrackedWindow { hwnd, visible });
    return TRUE;
}

} // anonymous namespace
