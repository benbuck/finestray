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
#include "MinimizePlacement.h"
#include "Log.h"

const char * minimizePlacementToCString(MinimizePlacement minimizePlacement)
{
    switch (minimizePlacement) {
        case MinimizePlacement::None: return "none";
        case MinimizePlacement::Tray: return "tray";
        case MinimizePlacement::Menu: return "menu";
        case MinimizePlacement::TrayAndMenu: return "tray-and-menu";

        default: {
            WARNING_PRINTF("error, bad minimize placement: %d\n", minimizePlacement);
            return "none";
        }
    }
}

MinimizePlacement minimizePlacementFromCString(const char * minimizePlacementString)
{
    if (!strcmp(minimizePlacementString, "none")) {
        return MinimizePlacement::None;
    }

    if (!strcmp(minimizePlacementString, "tray")) {
        return MinimizePlacement::Tray;
    }

    if (!strcmp(minimizePlacementString, "menu")) {
        return MinimizePlacement::Menu;
    }

    if (!strcmp(minimizePlacementString, "tray-and-menu")) {
        return MinimizePlacement::TrayAndMenu;
    }

    WARNING_PRINTF("error, bad minimize placement string: %s\n", minimizePlacementString);
    return MinimizePlacement::None;
}
