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

class MenuHandleWrapper
{
public:
    MenuHandleWrapper() = delete;

    explicit MenuHandleWrapper(HMENU hmenu) noexcept
        : hmenu_(hmenu)
    {
    }

    ~MenuHandleWrapper()
    {
        if (hmenu_) {
            if (!DestroyMenu(hmenu_)) {
                WARNING_PRINTF("failed to destroy menu %#x: %lu\n", hmenu_, GetLastError());
            }
        }
    }

    MenuHandleWrapper(const MenuHandleWrapper &) = delete;
    MenuHandleWrapper(MenuHandleWrapper &&) = delete;
    MenuHandleWrapper & operator=(const MenuHandleWrapper &) = delete;
    MenuHandleWrapper & operator=(MenuHandleWrapper &&) = delete;

    operator HMENU() const noexcept { return hmenu_; }

    operator bool() const noexcept { return hmenu_ != nullptr; }

private:
    HMENU hmenu_ {};
};
