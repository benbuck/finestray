// Copyright 2020 Benbuck Nason

#include "WindowList.h"

// App
#include "DebugPrint.h"
#include "StringUtility.h"

// Standard library
#include <set>

namespace WindowList
{

static VOID timerProc(HWND, UINT, UINT_PTR userData, DWORD);
static BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);

static HWND hwnd_;
static UINT pollMillis_;
static void (*addWindowCallback_)(HWND);
static void (*removeWindowCallback_)(HWND);
static UINT_PTR timer_;
static std::set<HWND> windowList_;

void start(HWND hwnd, UINT pollMillis, void (*addWindowCallback)(HWND), void (*removeWindowCallback)(HWND))
{
    hwnd_ = hwnd;
    pollMillis_ = pollMillis;
    addWindowCallback_ = addWindowCallback;
    removeWindowCallback_ = removeWindowCallback;

    if (pollMillis_ > 0) {
        timer_ = SetTimer(hwnd_, 1, pollMillis_, timerProc);
        if (!timer_) {
            DEBUG_PRINTF("SetTimer() failed: %s\n", StringUtility::lastErrorString().c_str());
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

VOID timerProc(HWND, UINT, UINT_PTR, DWORD)
{
    std::set<HWND> newWindowList;
    if (!EnumWindows(enumWindowsProc, (LPARAM)&newWindowList)) {
        DEBUG_PRINTF("could not list windows: EnumWindows() failed: %s\n", StringUtility::lastErrorString().c_str());
    }

    // check for removed windows
    if (removeWindowCallback_) {
        for (HWND hwnd : windowList_) {
            if (newWindowList.find(hwnd) == newWindowList.end()) {
                // removed window found
                removeWindowCallback_(hwnd);
            }
        }
    }

    // check for added windows
    if (addWindowCallback_) {
        for (HWND hwnd : newWindowList) {
            if (windowList_.find(hwnd) == windowList_.end()) {
                // added window found
                addWindowCallback_(hwnd);
            }
        }
    }

    // update the list
    windowList_ = newWindowList;
}

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd)) {
        // DEBUG_PRINTF("ignoring invisible window: %#x\n", hwnd);
        return TRUE;
    }

    std::set<HWND> & windowList = *(std::set<HWND> *)lParam;
    windowList.insert(hwnd);

    return TRUE;
}

} // namespace WindowList
