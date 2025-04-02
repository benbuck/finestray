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

class BrushHandleWrapper
{
public:
    BrushHandleWrapper() = default;

    explicit BrushHandleWrapper(HBRUSH hbrush) noexcept
        : hbrush_(hbrush)
    {
    }

    ~BrushHandleWrapper()
    {
        if (hbrush_) {
            if (!DeleteObject(hbrush_)) {
                WARNING_PRINTF("failed to destroy brush %#x\n", hbrush_);
            }
        }
    }

    BrushHandleWrapper(const BrushHandleWrapper &) = delete;
    BrushHandleWrapper(BrushHandleWrapper &&) = delete;
    BrushHandleWrapper & operator=(const BrushHandleWrapper &) = delete;
    BrushHandleWrapper & operator=(BrushHandleWrapper &&) = delete;

    operator HBRUSH() const noexcept { return hbrush_; }

    operator bool() const noexcept { return hbrush_ != nullptr; }

private:
    HBRUSH hbrush_ {};
};
