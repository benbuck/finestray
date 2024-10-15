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
#include "DebugPrint.h"

// Windows
#include <Windows.h>

class MenuWrapper
{
public:
    MenuWrapper(HMENU menu)
        : menu_(menu)
    {
    }

    ~MenuWrapper()
    {
        if (menu_) {
            if (!DestroyMenu(menu_)) {
                DEBUG_PRINTF("failed to destroy menu: %#x\n", menu_);
            }
        }
    }

    operator HMENU() const { return menu_; }

    operator bool() const { return menu_ != nullptr; }

private:
    HMENU menu_ = nullptr;
};
