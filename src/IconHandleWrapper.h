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

class IconHandleWrapper
{
public:
    enum Mode
    {
        Referenced,
        Created
    };

    IconHandleWrapper() = default;

    IconHandleWrapper(HICON hicon, Mode mode) noexcept
        : hicon_(hicon)
        , mode_(mode)
    {
        if (!hicon_) {
            ERROR_PRINTF("invalid icon handle: %#x\n", hicon);
        }
    }

    ~IconHandleWrapper()
    {
        if (hicon_ && (mode_ == Mode::Created)) {
            if (!DestroyIcon(hicon_)) {
                WARNING_PRINTF("DestroyIcon() failed: %lu\n", GetLastError());
            }
        }
    }

    IconHandleWrapper(const IconHandleWrapper &) = delete;
    IconHandleWrapper(IconHandleWrapper &&) = delete;
    IconHandleWrapper & operator=(const IconHandleWrapper &) = delete;

    IconHandleWrapper & operator=(IconHandleWrapper && other) noexcept
    {
        hicon_ = other.hicon_;
        mode_ = other.mode_;
        other.hicon_ = nullptr;
        other.mode_ = Mode::Referenced;
        return *this;
    }

    operator HICON() const noexcept { return hicon_; }

    operator bool() const noexcept { return hicon_ != nullptr; }

private:
    HICON hicon_ {};
    Mode mode_ {};
};
