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
#include "DebugPrint.h"
#include "StringUtility.h"

// Windows
#include <Windows.h>

class WinEventHookHandleWrapper
{
public:
    explicit WinEventHookHandleWrapper(HWINEVENTHOOK hwineventhook)
        : hwineventhook_(hwineventhook)
    {
    }

    ~WinEventHookHandleWrapper()
    {
        if (hwineventhook_) {
            if (!UnhookWinEvent(hwineventhook_)) {
                DEBUG_PRINTF(
                    "failed to unhook win event %#x, UnhookWinEvent() failed: %s\n",
                    hwineventhook_,
                    StringUtility::lastErrorString().c_str());
            }
        }
    }

    operator HWINEVENTHOOK() const { return hwineventhook_; }

private:
    HWINEVENTHOOK hwineventhook_;
};
