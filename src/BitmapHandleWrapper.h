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

class BitmapHandleWrapper
{
public:
    BitmapHandleWrapper() = default;

    explicit BitmapHandleWrapper(HBITMAP hbitmap)
        : hbitmap_(hbitmap)
    {
    }

    BitmapHandleWrapper(const BitmapHandleWrapper &) = delete;
    BitmapHandleWrapper & operator=(const BitmapHandleWrapper &) = delete;

    BitmapHandleWrapper(BitmapHandleWrapper && other) noexcept
        : hbitmap_(other.release())
    {
    }

    BitmapHandleWrapper & operator=(BitmapHandleWrapper && other) noexcept
    {
        if (this != &other) {
            destroy();

            hbitmap_ = other.release();
        }
        return *this;
    }

    ~BitmapHandleWrapper() { destroy(); }

    void destroy()
    {
        if (hbitmap_) {
            if (!DeleteObject(hbitmap_)) {
                WARNING_PRINTF("failed to destroy bitmap %#x\n", hbitmap_);
            }
            hbitmap_ = nullptr;
        }
    }

    operator HBITMAP() const { return hbitmap_; }

    operator bool() const { return hbitmap_ != nullptr; }

    HBITMAP release()
    {
        HBITMAP hbitmap = hbitmap_;
        hbitmap_ = nullptr;
        return hbitmap;
    }

private:
    HBITMAP hbitmap_ {};
};
