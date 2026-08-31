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
#include "Modifiers.h"
#include "Helpers.h"
#include "Log.h"

namespace
{

bool isKeyDown(int key) noexcept;
bool isKeyDownAsync(int key) noexcept;

} // anonymous namespace

bool modifiersActive(UINT modifiers) noexcept
{
    if (!modifiers) {
        return false;
    }

    if (modifiers & ~(narrow_cast<UINT>(MOD_ALT) | narrow_cast<UINT>(MOD_CONTROL) | narrow_cast<UINT>(MOD_SHIFT))) {
        WARNING_PRINTF("invalid modifiers: %#x\n", modifiers);
        return false;
    }

    if (modifiers & narrow_cast<UINT>(MOD_ALT)) {
        if (!isKeyDown(VK_MENU) && !isKeyDown(VK_LMENU) && !isKeyDown(VK_RMENU)) {
            DEBUG_PRINTF("\talt modifier not down\n");
            return false;
        }
    }

    if (modifiers & narrow_cast<UINT>(MOD_CONTROL)) {
        if (!isKeyDown(VK_CONTROL) && !isKeyDown(VK_LCONTROL) && !isKeyDown(VK_RCONTROL)) {
            DEBUG_PRINTF("\tctrl modifier not down\n");
            return false;
        }
    }

    if (modifiers & narrow_cast<UINT>(MOD_SHIFT)) {
        if (!isKeyDown(VK_SHIFT) && !isKeyDown(VK_LSHIFT) && !isKeyDown(VK_RSHIFT)) {
            DEBUG_PRINTF("\tshift modifier not down\n");
            return false;
        }
    }

    if (modifiers & narrow_cast<UINT>(MOD_WIN)) {
        if (!isKeyDown(VK_LWIN) && !isKeyDown(VK_RWIN)) {
            DEBUG_PRINTF("\twin modifier not down\n");
            return false;
        }
    }

    return true;
}

// Same as modifiersActive(), but checks the physical key state via GetAsyncKeyState().
bool modifiersActiveAsync(UINT modifiers) noexcept
{
    if (!modifiers) {
        return false;
    }

    if (modifiers & ~(narrow_cast<UINT>(MOD_ALT) | narrow_cast<UINT>(MOD_CONTROL) | narrow_cast<UINT>(MOD_SHIFT))) {
        return false;
    }

    if (modifiers & narrow_cast<UINT>(MOD_ALT)) {
        if (!isKeyDownAsync(VK_MENU) && !isKeyDownAsync(VK_LMENU) && !isKeyDownAsync(VK_RMENU)) {
            return false;
        }
    }

    if (modifiers & narrow_cast<UINT>(MOD_CONTROL)) {
        if (!isKeyDownAsync(VK_CONTROL) && !isKeyDownAsync(VK_LCONTROL) && !isKeyDownAsync(VK_RCONTROL)) {
            return false;
        }
    }

    if (modifiers & narrow_cast<UINT>(MOD_SHIFT)) {
        if (!isKeyDownAsync(VK_SHIFT) && !isKeyDownAsync(VK_LSHIFT) && !isKeyDownAsync(VK_RSHIFT)) {
            return false;
        }
    }

    if (modifiers & narrow_cast<UINT>(MOD_WIN)) {
        if (!isKeyDownAsync(VK_LWIN) && !isKeyDownAsync(VK_RWIN)) {
            return false;
        }
    }

    return true;
}

namespace
{

bool isKeyDown(int key) noexcept
{
    return (narrow_cast<UINT>(GetKeyState(key)) & 0x8000U) != 0;
}

bool isKeyDownAsync(int key) noexcept
{
    return (narrow_cast<UINT>(GetAsyncKeyState(key)) & 0x8000U) != 0;
}

} // anonymous namespace
