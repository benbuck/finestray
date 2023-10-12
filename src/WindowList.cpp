// Copyright 2020 Benbuck Nason

#include "WindowList.h"

#include "DebugPrint.h"

#include <set>

static VOID timerProc(HWND, UINT, UINT_PTR userData, DWORD);
static BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);

static HWND hwnd_;
static UINT intervalMs_;
static void (*newWindowCallback_)(HWND);
static UINT_PTR timer_;
static std::set<HWND> windowList_;

void windowListStart(HWND hwnd, UINT intervalMs, void (*newWindowCallback)(HWND))
{
    hwnd_ = hwnd;
    intervalMs_ = intervalMs;
    newWindowCallback_ = newWindowCallback;

    if (intervalMs_ > 0) {
        timer_ = SetTimer(hwnd_, 1, intervalMs_, timerProc);
        if (!timer_) {
            DEBUG_PRINTF("SetTimer() failed: %u\n", GetLastError());
        }
    }
}

void windowListStop()
{
    windowList_.clear();

    if (timer_) {
        KillTimer(hwnd_, timer_);
        timer_ = 0;
    }

    newWindowCallback_ = nullptr;
    intervalMs_ = 0;
    hwnd_ = nullptr;
}

VOID timerProc(HWND, UINT, UINT_PTR, DWORD)
{
    std::set<HWND> newWindowList;
    if (!EnumWindows(enumWindowsProc, (LPARAM)&newWindowList)) {
        DEBUG_PRINTF("EnumWindows() failed: %u\n", GetLastError());
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
