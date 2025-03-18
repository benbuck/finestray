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
    explicit BrushHandleWrapper(HBRUSH hbrush)
        : hbrush_(hbrush)
    {
    }

    ~BrushHandleWrapper()
    {
        if (hbrush_) {
            if (!DeleteObject(hbrush_)) {
                WARNING_PRINTF("failed to destroy brush %#x: %s\n", hbrush_, StringUtility::lastErrorString().c_str());
            }
        }
    }

    operator HBRUSH() const { return hbrush_; }

    operator bool() const { return hbrush_ != nullptr; }

private:
    HBRUSH hbrush_ {};
};
