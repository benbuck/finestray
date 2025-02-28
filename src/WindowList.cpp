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

#include "WindowList.h"

// App
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"

// Standard library
#include <map>

namespace
{

struct WindowData
{
    std::string title;
    bool visible;
};

HWND hwnd_;
UINT pollMillis_;
void (*addWindowCallback_)(HWND);
void (*removeWindowCallback_)(HWND);
void (*changeWindowTitleCallback_)(HWND, const std::string &);
void (*changeWindowVisibilityCallback_)(HWND, bool);
UINT_PTR timer_;
std::map<HWND, WindowData> windowList_;

VOID timerProc(HWND, UINT, UINT_PTR userData, DWORD);
BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);

} // anonymous namespace

namespace WindowList
{

void start(
    HWND hwnd,
    UINT pollMillis,
    void (*addWindowCallback)(HWND),
    void (*removeWindowCallback)(HWND),
    void (*changeWindowTitleCallback)(HWND, const std::string &),
    void (*changeVisibilityCallback)(HWND, bool))
{
    hwnd_ = hwnd;
    pollMillis_ = pollMillis;
    addWindowCallback_ = addWindowCallback;
    removeWindowCallback_ = removeWindowCallback;
    changeWindowTitleCallback_ = changeWindowTitleCallback;
    changeWindowVisibilityCallback_ = changeVisibilityCallback;

    if (pollMillis_ > 0) {
        timer_ = SetTimer(hwnd_, 1, pollMillis_, timerProc);
        if (!timer_) {
            WARNING_PRINTF("SetTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

void stop()
{
    windowList_.clear();

    if (timer_) {
        KillTimer(hwnd_, timer_);
        timer_ = 0;
    }

    removeWindowCallback_ = nullptr;
    addWindowCallback_ = nullptr;
    pollMillis_ = 0;
    hwnd_ = nullptr;
}

} // namespace WindowList

namespace
{

VOID timerProc(HWND, UINT, UINT_PTR, DWORD)
{
    std::map<HWND, WindowData> newWindowList;
    if (!EnumWindows(enumWindowsProc, (LPARAM)&newWindowList)) {
        WARNING_PRINTF("could not list windows: EnumWindows() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    // check for removed windows
    if (removeWindowCallback_) {
        for (std::pair<HWND, WindowData> window : windowList_) {
            if (newWindowList.find(window.first) == newWindowList.end()) {
                // removed window found
                removeWindowCallback_(window.first);
            }
        }
    }

    // check for added windows
    if (addWindowCallback_) {
        for (std::pair<HWND, WindowData> window : newWindowList) {
            if (windowList_.find(window.first) == windowList_.end()) {
                // added window found
                addWindowCallback_(window.first);
            }
        }
    }

    // check for changed window titles or visibility
    if (changeWindowTitleCallback_ || changeWindowVisibilityCallback_) {
        for (std::pair<HWND, WindowData> window : newWindowList) {
            auto it = windowList_.find(window.first);
            if (it != windowList_.end()) {
                // existing window found
                if (it->second.title != window.second.title) {
                    // window title changed
                    changeWindowTitleCallback_(window.first, window.second.title);
                } else if (it->second.visible != window.second.visible) {
                    // window visibility changed
                    changeWindowVisibilityCallback_(window.first, window.second.visible);
                }
            }
        }
    }

    // update the list
    windowList_ = newWindowList;
}

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd) && (windowList_.find(hwnd) == windowList_.end())) {
        // DEBUG_PRINTF("ignoring invisible window: %#x\n", hwnd);
        return TRUE;
    }

    std::string title = getWindowText(hwnd);
    if (title.empty()) {
        // DEBUG_PRINTF("ignoring window with empty title: %#x\n", hwnd);
        return TRUE;
    }

    std::map<HWND, WindowData> & windowList = *(std::map<HWND, WindowData> *)lParam;
    windowList[hwnd].title = title;
    windowList[hwnd].visible = IsWindowVisible(hwnd);

    return TRUE;
}

} // anonymous namespace
