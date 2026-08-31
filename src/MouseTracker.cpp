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
#include "MouseTracker.h"
#include "Helpers.h"
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <WinUser.h>

namespace
{

HHOOK mouseHook_ {};
MouseTracker::Callback callback_ {};

LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept;

} // anonymous namespace

namespace MouseTracker
{

bool start(Callback callback)
{
    if (mouseHook_) {
        return true;
    }

    DEBUG_PRINTF("right-click minimize hooking mouse input\n");

    callback_ = callback;

    HINSTANCE hinstance = getInstance();
    mouseHook_ = SetWindowsHookExA(WH_MOUSE_LL, mouseHookProc, hinstance, 0);
    if (!mouseHook_) {
        ERROR_PRINTF("SetWindowsHookExA() failed: %s\n", StringUtility::lastErrorString().c_str());
        callback_ = nullptr;
        return false;
    }

    return true;
}

void stop() noexcept
{
    DEBUG_PRINTF("right-click minimize stopping\n");

    if (mouseHook_) {
        if (!UnhookWindowsHookEx(mouseHook_)) {
            ERROR_PRINTF("UnhookWindowsHookEx() failed: %ld\n", GetLastError());
        }

        mouseHook_ = nullptr;
    }

    callback_ = nullptr;
}

} // namespace MouseTracker

namespace
{

LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept
{
    if ((nCode == HC_ACTION) && callback_) {
        if (callback_(nCode, wParam, lParam)) {
            // swallow the event so the window never sees it
            return 1;
        }
    }

    return CallNextHookEx(mouseHook_, nCode, wParam, lParam);
}

} // anonymous namespace
