// Copyright 2020 Benbuck Nason

#include "WindowList.h"

// MinTray
#include "DebugPrint.h"

// standard library
#include <set>

namespace WindowList
{

static VOID timerProc(HWND, UINT, UINT_PTR userData, DWORD);
static BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);

static HWND hwnd_;
static UINT pollMillis_;
static void (*newWindowCallback_)(HWND);
static UINT_PTR timer_;
static std::set<HWND> windowList_;

void start(HWND hwnd, UINT pollMillis, void (*newWindowCallback)(HWND))
{
    hwnd_ = hwnd;
    pollMillis_ = pollMillis;
    newWindowCallback_ = newWindowCallback;

    if (pollMillis_ > 0) {
        timer_ = SetTimer(hwnd_, 1, pollMillis_, timerProc);
        if (!timer_) {
            DEBUG_PRINTF("SetTimer() failed: %u\n", GetLastError());
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

    newWindowCallback_ = nullptr;
    pollMillis_ = 0;
    hwnd_ = nullptr;
}

VOID timerProc(HWND, UINT, UINT_PTR, DWORD)
{
    std::set<HWND> newWindowList;
    if (!EnumWindows(enumWindowsProc, (LPARAM)&newWindowList)) {
        DEBUG_PRINTF("could not list windows: EnumWindows() failed: %u\n", GetLastError());
    }

    // Check for new windows
    for (auto const & hwnd : newWindowList) {
        if (windowList_.find(hwnd) == windowList_.end()) {
            // New window found
            newWindowCallback_(hwnd);
        }
    }

    // Update the list
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
