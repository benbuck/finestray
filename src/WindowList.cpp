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
#include "WindowList.h"
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"
#include "WindowIcon.h"

using WindowList::WindowData;

namespace
{

HWND hwnd_;
UINT pollMillis_;
void (*addWindowCallback_)(HWND);
void (*removeWindowCallback_)(HWND);
void (*changeWindowTitleCallback_)(HWND, const std::string &);
void (*changeWindowVisibilityCallback_)(HWND, bool);
UINT_PTR timer_;
std::map<HWND, WindowData> windowList_;

VOID timerProc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);
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
    DEBUG_PRINTF("WindowList starting\n");

    hwnd_ = hwnd;
    pollMillis_ = pollMillis;
    addWindowCallback_ = addWindowCallback;
    removeWindowCallback_ = removeWindowCallback;
    changeWindowTitleCallback_ = changeWindowTitleCallback;
    changeWindowVisibilityCallback_ = changeVisibilityCallback;

    if (pollMillis_ > 0) {
        DEBUG_PRINTF("WindowList setting poll timer to %u\n", pollMillis_);
        timer_ = SetTimer(hwnd_, 1, pollMillis_, timerProc);
        if (!timer_) {
            ERROR_PRINTF("SetTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
        }
    }
}

void stop()
{
    DEBUG_PRINTF("WindowList stopping\n");

    windowList_.clear();

    if (timer_) {
        if (!KillTimer(hwnd_, timer_)) {
            ERROR_PRINTF("KillTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
        }
        timer_ = 0;
    }

    removeWindowCallback_ = nullptr;
    addWindowCallback_ = nullptr;
    pollMillis_ = 0;
    hwnd_ = nullptr;
}

std::map<HWND, WindowData> getAll()
{
    return windowList_;
}

HWND getVisibleIndex(unsigned int index)
{
    if (index >= windowList_.size()) {
        return nullptr;
    }

    unsigned int count = 0;
    for (const std::pair<HWND, WindowList::WindowData> window : windowList_) {
        if (window.second.visible) {
            if (count == index) {
                return window.first;
            }
            ++count;
        }
    }

    return nullptr;
}

} // namespace WindowList

namespace
{

VOID timerProc(
    HWND /* unnamedParam1 */,
    UINT /* unnamedParam2 */,
    UINT_PTR /* unnamedParam3 */,
    DWORD /* unnamedParam4 */)
{
    std::map<HWND, WindowData> newWindowList;
    if (!EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&newWindowList))) {
        ERROR_PRINTF("could not list windows: EnumWindows() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    // check for removed windows
    if (removeWindowCallback_) {
        for (const std::pair<HWND, WindowData> window : windowList_) {
            if (newWindowList.find(window.first) == newWindowList.end()) {
                // removed window found
                removeWindowCallback_(window.first);
            }
        }
    }

    // check for added windows
    if (addWindowCallback_) {
        for (const std::pair<HWND, WindowData> window : newWindowList) {
            if (windowList_.find(window.first) == windowList_.end()) {
                // added window found
                addWindowCallback_(window.first);
            }
        }
    }

    // check for changed window titles or visibility
    if (changeWindowTitleCallback_ || changeWindowVisibilityCallback_) {
        for (const std::pair<HWND, WindowData> window : newWindowList) {
            const std::map<HWND, WindowData>::const_iterator it = windowList_.find(window.first);
            if (it != windowList_.end()) {
                // existing window found
                if (it->second.title != window.second.title) {
                    // window title changed
                    if (changeWindowTitleCallback_) {
                        changeWindowTitleCallback_(window.first, window.second.title);
                    }
                } else if (it->second.visible != window.second.visible) {
                    // window visibility changed
                    if (changeWindowVisibilityCallback_) {
                        changeWindowVisibilityCallback_(window.first, window.second.visible);
                    }
                }
            }
        }
    }

    // update the list
    windowList_.swap(newWindowList);
}

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    std::map<HWND, WindowData> & windowList = *reinterpret_cast<std::map<HWND, WindowData> *>(lParam);
    windowList[hwnd].title = getWindowText(hwnd);
    windowList[hwnd].visible = isWindowUserVisible(hwnd);

    return TRUE;
}

} // anonymous namespace
