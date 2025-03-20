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
#include "ContextMenu.h"
#include "AppName.h"
#include "Bitmap.h"
#include "BitmapHandleWrapper.h"
#include "Helpers.h"
#include "Log.h"
#include "MenuHandleWrapper.h"
#include "MinimizePlacement.h"
#include "MinimizedWindow.h"
#include "Resource.h"
#include "StringUtility.h"
#include "WindowIcon.h"
#include "WindowList.h"

namespace
{

bool addMenuItemForWindow(HMENU menu, HWND hwnd, unsigned int id);

} // anonymous namespace

bool showContextMenu(HWND hwnd, MinimizePlacement minimizePlacement, bool showWindows)
{
    // create popup menu
    MenuHandleWrapper menu(CreatePopupMenu());
    if (!menu) {
        WARNING_PRINTF(
            "failed to create context menu, CreatePopupMenu() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // add menu entries
    if (!AppendMenuA(menu, MF_STRING, IDM_APP, APP_NAME)) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }
    if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    std::map<HWND, WindowList::WindowData> windowList = WindowList::getAll();
    if (showWindows) {
        unsigned int visibleCount = 0;
        for (const std::pair<HWND, WindowList::WindowData> & window : windowList) {
            if (window.second.visible) {
                if (!addMenuItemForWindow(menu, window.first, IDM_VISIBLEWINDOW_BASE + visibleCount)) {
                    return false;
                }

                ++visibleCount;
            }
        }

        if (visibleCount) {
            if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }
            if (!AppendMenuA(menu, MF_STRING, IDM_MINIMIZE_ALL, getResourceString(IDS_MENU_MINIMIZE_ALL).c_str())) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }
        }

        if (visibleCount) {
            if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }
        }
    }

    std::vector<HWND> minimizedWindows = MinimizedWindow::getAll();

    if (minimizePlacementIncludesMenu(minimizePlacement)) {
        unsigned int minimizedCount = 0;
        for (HWND minimizedWindow : minimizedWindows) {
            if (!addMenuItemForWindow(menu, minimizedWindow, IDM_MINIMIZEDWINDOW_BASE + minimizedCount)) {
                return false;
            }

            ++minimizedCount;
        }
        if (minimizedCount) {
            if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }
        }
    }

    if (!minimizedWindows.empty()) {
        if (!AppendMenuA(menu, MF_STRING, IDM_RESTORE_ALL, getResourceString(IDS_MENU_RESTORE_ALL).c_str())) {
            WARNING_PRINTF(
                "failed to create menu entry, AppendMenuA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
            return false;
        }
        if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
            WARNING_PRINTF(
                "failed to create menu entry, AppendMenuA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
            return false;
        }
    }

    if (!AppendMenuA(menu, MF_STRING, IDM_SETTINGS, getResourceString(IDS_MENU_SETTINGS).c_str())) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    if (!AppendMenuA(menu, MF_STRING, IDM_EXIT, getResourceString(IDS_MENU_EXIT).c_str())) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    BitmapHandleWrapper appBitmap(Bitmap::getResource(IDB_APP));
    BitmapHandleWrapper minimizeBitmap(Bitmap::getResource(IDB_MINIMIZE));
    BitmapHandleWrapper restoreBitmap(Bitmap::getResource(IDB_RESTORE));
    BitmapHandleWrapper settingsBitmap(Bitmap::getResource(IDB_SETTINGS));
    BitmapHandleWrapper exitBitmap(Bitmap::getResource(IDB_EXIT));

    if (!appBitmap || !minimizeBitmap || !restoreBitmap || !settingsBitmap || !exitBitmap) {
        WARNING_PRINTF("failed to load bitmap: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        COLORREF oldColor1 = RGB(0xFF, 0xFF, 0xFF);
        COLORREF oldColor2 = RGB(0x00, 0x00, 0x00);
        DWORD menuColor = GetSysColor(COLOR_MENU);
        COLORREF newColor = RGB(GetBValue(menuColor), GetGValue(menuColor), GetRValue(menuColor));

        Bitmap::replaceColor(appBitmap, oldColor1, newColor);
        Bitmap::replaceColor(settingsBitmap, oldColor1, newColor);
        Bitmap::replaceColor(exitBitmap, oldColor1, newColor);

        Bitmap::replaceColor(appBitmap, oldColor2, newColor);
        Bitmap::replaceColor(settingsBitmap, oldColor2, newColor);
        Bitmap::replaceColor(exitBitmap, oldColor2, newColor);

        MENUITEMINFOA menuItemInfo;
        memset(&menuItemInfo, 0, sizeof(MENUITEMINFOA));
        menuItemInfo.cbSize = sizeof(MENUITEMINFOA);
        menuItemInfo.fMask = MIIM_BITMAP;

        menuItemInfo.hbmpItem = appBitmap;
        // FIX - why doesn't this work?
        // menuItemInfo.hbmpItem = HBMMENU_SYSTEM;
        // menuItemInfo.dwItemData = (ULONG_PTR)(HWND)appWindow_;
        // DEBUG_PRINTF("dwItemData %x, app window: %x\n", menuItemInfo.dwItemData, (HWND)appWindow_);
        if (!SetMenuItemInfoA(menu, IDM_APP, FALSE, &menuItemInfo)) {
            WARNING_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        if (!windowList.empty()) {
            menuItemInfo.hbmpItem = minimizeBitmap;
            if (!SetMenuItemInfoA(menu, IDM_MINIMIZE_ALL, FALSE, &menuItemInfo)) {
                WARNING_PRINTF(
                    "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
            }
        }

        if (!minimizedWindows.empty()) {
            menuItemInfo.hbmpItem = restoreBitmap;
            if (!SetMenuItemInfoA(menu, IDM_RESTORE_ALL, FALSE, &menuItemInfo)) {
                WARNING_PRINTF(
                    "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
            }
        }

        menuItemInfo.hbmpItem = settingsBitmap;
        if (!SetMenuItemInfoA(menu, IDM_SETTINGS, FALSE, &menuItemInfo)) {
            WARNING_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }

        menuItemInfo.hbmpItem = exitBitmap;
        if (!SetMenuItemInfoA(menu, IDM_EXIT, FALSE, &menuItemInfo)) {
            WARNING_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
        }
    }

    // activate our window
    if (!SetForegroundWindow(hwnd)) {
        WARNING_PRINTF(
            "failed to activate context menu, SetForegroundWindow() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // get the current mouse position
    POINT point = { 0, 0 };
    if (!GetCursorPos(&point)) {
        WARNING_PRINTF(
            "failed to get menu position, GetCursorPos() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // show the popup menu
    if (!TrackPopupMenu(menu, 0, point.x, point.y, 0, hwnd, nullptr)) {
        WARNING_PRINTF(
            "failed to show context menu, TrackPopupMenu() failed: %s\n",
            StringUtility::lastErrorString().c_str());
        return false;
    }

    // force a task switch to our app
    if (!PostMessage(hwnd, WM_USER, 0, 0)) {
        WARNING_PRINTF("failed to activate app, PostMessage() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    return true;
}

namespace
{

bool addMenuItemForWindow(HMENU menu, HWND hwnd, unsigned int id)
{
    std::string title = getWindowText(hwnd);
    constexpr size_t maxTitleLength = 30;
    if (title.length() > maxTitleLength) {
        std::string_view ellipsis = "...";
        title.resize(maxTitleLength - ellipsis.length());
        title += ellipsis; // FIX - localize this?
    }
    if (!AppendMenuA(menu, MF_STRING, id, title.c_str())) {
        WARNING_PRINTF("failed to create menu entry, AppendMenuA() failed: %s\n", StringUtility::lastErrorString().c_str());
        return false;
    }

    HBITMAP hbmp = WindowIcon::bitmap(hwnd);
    if (hbmp) {
        MENUITEMINFOA menuItemInfo;
        memset(&menuItemInfo, 0, sizeof(MENUITEMINFOA));
        menuItemInfo.cbSize = sizeof(MENUITEMINFOA);
        menuItemInfo.fMask = MIIM_BITMAP;
        menuItemInfo.hbmpItem = hbmp;
        if (!SetMenuItemInfoA(menu, id, FALSE, &menuItemInfo)) {
            WARNING_PRINTF(
                "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                StringUtility::lastErrorString().c_str());
            return false;
        }
    }

    return true;
}

} // anonymous namespace
