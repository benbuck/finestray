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
#include "Log.h"

bool modifiersActive(UINT modifiers) noexcept
{
    if (!modifiers) {
        return false;
    }

    if (modifiers & ~(static_cast<UINT>(MOD_ALT) | static_cast<UINT>(MOD_CONTROL) | static_cast<UINT>(MOD_SHIFT))) {
        WARNING_PRINTF("invalid modifiers: %#x\n", modifiers);
        return false;
    }

    if (modifiers & static_cast<UINT>(MOD_ALT)) {
        if (!(static_cast<UINT>(GetKeyState(VK_MENU)) & 0x8000U) &&
            !(static_cast<UINT>(GetKeyState(VK_LMENU)) & 0x8000U) &&
            !(static_cast<UINT>(GetKeyState(VK_RMENU)) & 0x8000U)) {
            DEBUG_PRINTF("\talt modifier not down\n");
            return false;
        }
    }

    if (modifiers & static_cast<UINT>(MOD_CONTROL)) {
        if (!(static_cast<UINT>(GetKeyState(VK_CONTROL)) & 0x8000U) &&
            !(static_cast<UINT>(GetKeyState(VK_LCONTROL)) & 0x8000U) &&
            !(static_cast<UINT>(GetKeyState(VK_RCONTROL)) & 0x8000U)) {
            DEBUG_PRINTF("\tctrl modifier not down\n");
            return false;
        }
    }

    if (modifiers & static_cast<UINT>(MOD_SHIFT)) {
        if (!(static_cast<UINT>(GetKeyState(VK_SHIFT)) & 0x8000U) &&
            !(static_cast<UINT>(GetKeyState(VK_LSHIFT)) & 0x8000U) &&
            !(static_cast<UINT>(GetKeyState(VK_RSHIFT)) & 0x8000U)) {
            DEBUG_PRINTF("\tshift modifier not down\n");
            return false;
        }
    }

    if (modifiers & static_cast<UINT>(MOD_WIN)) {
        if (!(static_cast<UINT>(GetKeyState(VK_LWIN)) & 0x8000U) && !(static_cast<UINT>(GetKeyState(VK_RWIN)) & 0x8000U)) {
            DEBUG_PRINTF("\twin modifier not down\n");
            return false;
        }
    }

    return true;
}
