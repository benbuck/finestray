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

// Standard library
#include <vector>

class DeviceContextHandleWrapper
{
public:
    enum Mode
    {
        Referenced,
        Created
    };

    DeviceContextHandleWrapper() = delete;

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
            for (std::vector<HGDIOBJ>::reverse_iterator it = objects_.rbegin(); it != objects_.rend(); ++it) {
                SelectObject(hdc_, *it);
            }

            switch (mode_) {
                case Created: {
                    if (!DeleteDC(hdc_)) {
                        WARNING_PRINTF("DeleteDC() failed: %s\n", StringUtility::lastErrorString().c_str());
                    }
                    break;
                }
                case Referenced: {
                    if (!ReleaseDC(HWND_DESKTOP, hdc_)) {
                        WARNING_PRINTF("ReleaseDC() failed: %s\n", StringUtility::lastErrorString().c_str());
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

    DeviceContextHandleWrapper(const DeviceContextHandleWrapper &) = delete;
    DeviceContextHandleWrapper(DeviceContextHandleWrapper &&) = delete;
    DeviceContextHandleWrapper & operator=(const DeviceContextHandleWrapper &) = delete;
    DeviceContextHandleWrapper & operator=(DeviceContextHandleWrapper &&) = delete;

    operator HDC() const { return hdc_; }

    operator bool() const { return hdc_ != nullptr; }

    bool selectObject(HGDIOBJ object)
    {
        if (!hdc_) {
            return false;
        }

        HGDIOBJ oldObject = SelectObject(hdc_, object);
        if (!oldObject) {
            WARNING_PRINTF("SelectObject() failed: %s\n", StringUtility::lastErrorString().c_str());
            return false;
        }

        objects_.push_back(oldObject);
        return true;
    }

private:
    HDC hdc_ {};
    Mode mode_ {};
    std::vector<HGDIOBJ> objects_;
};
