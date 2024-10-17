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

class WindowHandleWrapper
{
public:
    WindowHandleWrapper()
        : hwnd_(nullptr)
    {
    }

    WindowHandleWrapper(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    ~WindowHandleWrapper() { destroy(); }

    void operator=(HWND hwnd)
    {
        destroy();

        hwnd_ = hwnd;
    }

    void destroy()
    {
        if (hwnd_) {
            if (!DestroyWindow(hwnd_)) {
                DEBUG_PRINTF("DestroyWindow() failed: %s\n", StringUtility::lastErrorString().c_str());
            }
        }
    }

    operator HWND() const { return hwnd_; }

private:
    HWND hwnd_;
};
