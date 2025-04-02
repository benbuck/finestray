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

#pragma once

// App
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <Windows.h>

class WinEventHookHandleWrapper
{
public:
    WinEventHookHandleWrapper() = delete;

    explicit WinEventHookHandleWrapper(HWINEVENTHOOK hwineventhook) noexcept
        : hwineventhook_(hwineventhook)
    {
    }

    ~WinEventHookHandleWrapper() { destroy(); }

    WinEventHookHandleWrapper(const WinEventHookHandleWrapper &) = delete;
    WinEventHookHandleWrapper(WinEventHookHandleWrapper &&) = delete;
    WinEventHookHandleWrapper & operator=(const WinEventHookHandleWrapper &) = delete;
    WinEventHookHandleWrapper & operator=(WinEventHookHandleWrapper &&) = delete;

    void destroy() noexcept
    {
        if (hwineventhook_) {
            DEBUG_PRINTF("destroying win event hook %#x\n", hwineventhook_);
            if (!UnhookWinEvent(hwineventhook_)) {
                WARNING_PRINTF(
                    "failed to unhook win event %#x, UnhookWinEvent() failed: %lu\n",
                    hwineventhook_,
                    GetLastError());
                return;
            }

            hwineventhook_ = nullptr;
        }
    }

    operator HWINEVENTHOOK() const noexcept { return hwineventhook_; }

private:
    HWINEVENTHOOK hwineventhook_ {};
};
