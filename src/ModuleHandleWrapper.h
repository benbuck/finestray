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

class ModuleHandleWrapper
{
public:
    ModuleHandleWrapper() = default;

    explicit ModuleHandleWrapper(HMODULE hmodule) noexcept
        : hmodule_(hmodule)
    {
    }

    ~ModuleHandleWrapper() { close(); }

    ModuleHandleWrapper & operator=(ModuleHandleWrapper && other) noexcept
    {
        close();

        hmodule_ = other.hmodule_;
        other.hmodule_ = nullptr;

        return *this;
    }

    ModuleHandleWrapper(const ModuleHandleWrapper &) = delete;
    ModuleHandleWrapper(ModuleHandleWrapper &&) = delete;
    ModuleHandleWrapper & operator=(const ModuleHandleWrapper &) = delete;

    void close() noexcept
    {
        if (hmodule_ != nullptr) {
            if (!FreeLibrary(hmodule_)) {
                WARNING_PRINTF("failed to close free library %#x: %lu\n", hmodule_, GetLastError());
                return;
            }

            hmodule_ = nullptr;
        }
    }

    operator HMODULE() const noexcept { return hmodule_; }

    operator bool() const noexcept { return hmodule_ != nullptr; }

private:
    HMODULE hmodule_ { nullptr };
};
