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

// Windows
#include <Windows.h>

class DeviceContextHandleWrapper
{
public:
    enum Mode
    {
        Referenced,
        Created
    };

    DeviceContextHandleWrapper(HDC hdc, Mode mode)
        : hdc_(hdc)
        , mode_(mode)
    {
        if (!hdc_) {
            ERROR_PRINTF("invalid device context handle: %#x\n", hdc);
        }
    }

    ~DeviceContextHandleWrapper()
    {
        if (hdc_) {
            switch (mode_) {
                case Created: {
                    if (!DeleteDC(hdc_)) {
                        WARNING_PRINTF("failed to delete device context: %#x\n", hdc_);
                    }
                    break;
                }
                case Referenced: {
                    if (!ReleaseDC(HWND_DESKTOP, hdc_)) {
                        WARNING_PRINTF("failed to release device context: %#x\n", hdc_);
                    }
                    break;
                }
                default: {
                    ERROR_PRINTF("unknown device context mode: %d\n", mode_);
                    break;
                }
            }
        }
    }

    operator HDC() const { return hdc_; }

    operator bool() const { return hdc_ != nullptr; }

private:
    HDC hdc_ = nullptr;
    Mode mode_;
};
