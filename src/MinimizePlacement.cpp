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
#include "DebugPrint.h"

std::string minimizePlacementToString(MinimizePlacement minimizePlacement)
{
    switch (minimizePlacement) {
        case MinimizePlacement::None: return "none";
        case MinimizePlacement::Tray: return "tray";
        case MinimizePlacement::Menu: return "menu";
        case MinimizePlacement::TrayAndMenu: return "tray-and-menu";

        default: {
            DEBUG_PRINTF("error, bad minimize placement: %d\n", minimizePlacement);
            return "none";
        }
    }
}

MinimizePlacement minimizePlacementFromString(const std::string & minimizePlacementString)
{
    if (minimizePlacementString == "none") {
        return MinimizePlacement::None;
    } else if (minimizePlacementString == "tray") {
        return MinimizePlacement::Tray;
    } else if (minimizePlacementString == "menu") {
        return MinimizePlacement::Menu;
    } else if (minimizePlacementString == "tray-and-menu") {
        return MinimizePlacement::TrayAndMenu;
    } else {
        DEBUG_PRINTF("error, bad minimize placement string: %s\n", minimizePlacementString.c_str());
        return MinimizePlacement::None;
    }
}

bool minimizePlacementIncludesTray(MinimizePlacement minimizePlacement)
{
    return (minimizePlacement == MinimizePlacement::Tray) || (minimizePlacement == MinimizePlacement::TrayAndMenu);
}

bool minimizePlacementIncludesMenu(MinimizePlacement minimizePlacement)
{
    return (minimizePlacement == MinimizePlacement::Menu) || (minimizePlacement == MinimizePlacement::TrayAndMenu);
}
