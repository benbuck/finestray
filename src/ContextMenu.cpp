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

bool showContextMenu(HWND hwnd, MinimizePlacement minimizePlacement)
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

    if (minimizePlacementIncludesMenu(minimizePlacement)) {
        unsigned int count = 0;
        std::vector<HWND> minimizedWindows = MinimizedWindow::getAll();
        for (HWND minimizedWindow : minimizedWindows) {
            std::string title = getWindowText(minimizedWindow);
            constexpr size_t maxTitleLength = 30;
            if (title.length() > maxTitleLength) {
                std::string_view ellipsis = "...";
                title.resize(maxTitleLength - ellipsis.length());
                title += ellipsis; // FIX - localize this?
            }
            if (!AppendMenuA(menu, MF_STRING, IDM_MINIMIZEDWINDOW_BASE + count, title.c_str())) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }

            HBITMAP hbmp = WindowIcon::bitmap(minimizedWindow);
            if (hbmp) {
                MENUITEMINFOA menuItemInfo;
                memset(&menuItemInfo, 0, sizeof(MENUITEMINFOA));
                menuItemInfo.cbSize = sizeof(MENUITEMINFOA);
                menuItemInfo.fMask = MIIM_BITMAP;
                menuItemInfo.hbmpItem = hbmp;
                if (!SetMenuItemInfoA(menu, IDM_MINIMIZEDWINDOW_BASE + count, FALSE, &menuItemInfo)) {
                    WARNING_PRINTF(
                        "failed to create menu entry, SetMenuItemInfoA() failed: %s\n",
                        StringUtility::lastErrorString().c_str());
                    return false;
                }
            }

            ++count;
        }
        if (count) {
            if (!AppendMenuA(menu, MF_SEPARATOR, 0, nullptr)) {
                WARNING_PRINTF(
                    "failed to create menu entry, AppendMenuA() failed: %s\n",
                    StringUtility::lastErrorString().c_str());
                return false;
            }
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

    BitmapHandleWrapper appBitmap(getResourceBitmap(IDB_APP));
    BitmapHandleWrapper settingsBitmap(getResourceBitmap(IDB_SETTINGS));
    BitmapHandleWrapper exitBitmap(getResourceBitmap(IDB_EXIT));

    if (!appBitmap || !settingsBitmap || !exitBitmap) {
        WARNING_PRINTF("failed to load bitmap: %s\n", StringUtility::lastErrorString().c_str());
    } else {
        COLORREF oldColor = RGB(0xFF, 0xFF, 0xFF);
        DWORD menuColor = GetSysColor(COLOR_MENU);
        COLORREF newColor = RGB(GetBValue(menuColor), GetGValue(menuColor), GetRValue(menuColor));
        replaceBitmapColor(appBitmap, oldColor, newColor);
        replaceBitmapColor(settingsBitmap, oldColor, newColor);
        replaceBitmapColor(exitBitmap, oldColor, newColor);

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
