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

#include "TrayWindow.h"

// App
#include "DebugPrint.h"
#include "StringUtility.h"
#include "TrayIcon.h"

// Standard library
#include <list>
#include <memory>

namespace TrayWindow
{

struct IconData
{
    IconData(HWND hwnd, TrayIcon * trayIcon)
        : hwnd_(hwnd)
        , trayIcon_(trayIcon)
    {
    }

    HWND hwnd_;
    std::unique_ptr<TrayIcon> trayIcon_;
};

typedef std::list<IconData> TrayIcons;
static TrayIcons trayIcons_;

static TrayIcons::iterator find(HWND hwnd);
static bool add(HWND hwnd, HWND messageHwnd);
static bool remove(HWND hwnd);

void minimize(HWND hwnd, HWND messageHwnd)
{
    DEBUG_PRINTF("tray window minimize %#x\n", hwnd);

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

    // add tray icon
    if (find(hwnd) == trayIcons_.end()) {
        if (!add(hwnd, messageHwnd)) {
            // un-hide window on failure, but leave it minimized

            DEBUG_PRINTF("failed to add tray icon for %#x\n", hwnd);

            if (!ShowWindow(hwnd, SW_SHOW)) {
                DEBUG_PRINTF(
                    "failed to show window %#x, ShowWindow() failed: %s\n",
                    hwnd,
                    StringUtility::lastErrorString().c_str());
            }

            return;
        }
    }
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

    remove(hwnd);
}

void close(HWND hwnd)
{
    DEBUG_PRINTF("tray window close %#x\n", hwnd);

    // close the window
    if (!PostMessage(hwnd, WM_CLOSE, 0, 0)) {
        DEBUG_PRINTF(
            "failed to post close to %#x, PostMessage() failed: %s\n",
            hwnd,
            StringUtility::lastErrorString().c_str());
    }

    // wait for close to complete
    unsigned int counter = 0;
    do {
        Sleep(100);
        ++counter;
        if (counter >= 10) {
            break;
        }
    } while (IsWindow(hwnd));

    // closed successfully
    if (!IsWindow(hwnd)) {
        remove(hwnd);
    }
}

void addAll()
{
    for (TrayIcons::const_iterator cit = trayIcons_.cbegin(); cit != trayIcons_.cend(); ++cit) {
        const IconData & iconData = *cit;
        add(iconData.hwnd_, iconData.trayIcon_->hwnd());
    }
}

void restoreAll()
{
    while (!trayIcons_.empty()) {
        restore(trayIcons_.front().hwnd_);
    }
}

HWND getFromID(UINT id)
{
    for (TrayIcons::const_iterator cit = trayIcons_.cbegin(); cit != trayIcons_.cend(); ++cit) {
        const IconData & iconData = *cit;
        if (iconData.trayIcon_->id() == id) {
            return iconData.hwnd_;
        }
    }

    return nullptr;
}

HWND getLast()
{
    if (trayIcons_.empty()) {
        return nullptr;
    }

    return trayIcons_.back().hwnd_;
}

HICON getIcon(HWND hwnd)
{
    HICON hicon;

    hicon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL2, 0);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
    if (hicon) {
        return hicon;
    }

    hicon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    if (hicon) {
        return hicon;
    }

    return nullptr;
}

TrayIcons::iterator find(HWND hwnd)
{
    for (TrayIcons::iterator it = trayIcons_.begin(); it != trayIcons_.end(); ++it) {
        const IconData & iconData = *it;
        if (iconData.hwnd_ == hwnd) {
            return it;
        }
    }

    return trayIcons_.end();
}

bool exists(HWND hwnd)
{
    return find(hwnd) != trayIcons_.end();
}

bool add(HWND hwnd, HWND messageHwnd)
{
    // make sure window isn't already tracked
    TrayIcons::iterator it = find(hwnd);
    if (it != trayIcons_.end()) {
        DEBUG_PRINTF("not adding already tracked tray window %#x\n", hwnd);
        return false;
    }

    // add the window
    trayIcons_.emplace_back(hwnd, new TrayIcon());
    HICON hicon = getIcon(hwnd);
    if (!hicon) {
        hicon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    return trayIcons_.back().trayIcon_->create(messageHwnd, WM_TRAYWINDOW, hicon);
}

bool remove(HWND hwnd)
{
    // find the window
    TrayIcons::iterator it = find(hwnd);
    if (it == trayIcons_.end()) {
        DEBUG_PRINTF("failed to remove unknown tray window %#x\n", hwnd);
        return false;
    }

    // remove the window
    trayIcons_.erase(it);
    return true;
}

} // namespace TrayWindow
