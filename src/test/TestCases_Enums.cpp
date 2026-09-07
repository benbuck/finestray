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
#include "MinimizePersistence.h"
#include "MinimizePlacement.h"
#include "TrayEvent.h"

// doctest
#include <doctest/doctest.h>

TEST_CASE("TrayEvent cstring round-trip")
{
    CHECK(trayEventFromCString(trayEventToCString(TrayEvent::Open)) == TrayEvent::Open);
    CHECK(trayEventFromCString(trayEventToCString(TrayEvent::Minimize)) == TrayEvent::Minimize);
    CHECK(trayEventFromCString(trayEventToCString(TrayEvent::OpenAndMinimize)) == TrayEvent::OpenAndMinimize);
    CHECK(trayEventFromCString(trayEventToCString(TrayEvent::None)) == TrayEvent::None);
    CHECK(trayEventFromCString("bogus") == TrayEvent::None);
}

TEST_CASE("TrayEvent valid/includes")
{
    CHECK(trayEventValid(TrayEvent::Open));
    CHECK(trayEventValid(TrayEvent::Minimize));
    CHECK(trayEventValid(TrayEvent::OpenAndMinimize));
    CHECK(!trayEventValid(TrayEvent::None));

    CHECK(trayEventIncludesOpen(TrayEvent::Open));
    CHECK(trayEventIncludesOpen(TrayEvent::OpenAndMinimize));
    CHECK(!trayEventIncludesOpen(TrayEvent::Minimize));
    CHECK(!trayEventIncludesOpen(TrayEvent::None));

    CHECK(trayEventIncludesMinimize(TrayEvent::Minimize));
    CHECK(trayEventIncludesMinimize(TrayEvent::OpenAndMinimize));
    CHECK(!trayEventIncludesMinimize(TrayEvent::Open));
    CHECK(!trayEventIncludesMinimize(TrayEvent::None));
}

TEST_CASE("MinimizePlacement cstring round-trip")
{
    CHECK(minimizePlacementFromCString(minimizePlacementToCString(MinimizePlacement::Tray)) == MinimizePlacement::Tray);
    CHECK(minimizePlacementFromCString(minimizePlacementToCString(MinimizePlacement::Menu)) == MinimizePlacement::Menu);
    CHECK(
        minimizePlacementFromCString(minimizePlacementToCString(MinimizePlacement::TrayAndMenu)) ==
        MinimizePlacement::TrayAndMenu);
    CHECK(minimizePlacementFromCString("bogus") == MinimizePlacement::None);
}

TEST_CASE("MinimizePlacement valid/includes")
{
    CHECK(minimizePlacementValid(MinimizePlacement::Tray));
    CHECK(minimizePlacementValid(MinimizePlacement::Menu));
    CHECK(minimizePlacementValid(MinimizePlacement::TrayAndMenu));
    CHECK(!minimizePlacementValid(MinimizePlacement::None));

    CHECK(minimizePlacementIncludesTray(MinimizePlacement::Tray));
    CHECK(minimizePlacementIncludesTray(MinimizePlacement::TrayAndMenu));
    CHECK(!minimizePlacementIncludesTray(MinimizePlacement::Menu));

    CHECK(minimizePlacementIncludesMenu(MinimizePlacement::Menu));
    CHECK(minimizePlacementIncludesMenu(MinimizePlacement::TrayAndMenu));
    CHECK(!minimizePlacementIncludesMenu(MinimizePlacement::Tray));
}

TEST_CASE("MinimizePersistence cstring round-trip")
{
    CHECK(
        minimizePersistenceFromCString(minimizePersistenceToCString(MinimizePersistence::Never)) ==
        MinimizePersistence::Never);
    CHECK(
        minimizePersistenceFromCString(minimizePersistenceToCString(MinimizePersistence::Always)) ==
        MinimizePersistence::Always);
    CHECK(minimizePersistenceFromCString("bogus") == MinimizePersistence::None);
}

TEST_CASE("MinimizePersistence valid")
{
    CHECK(minimizePersistenceValid(MinimizePersistence::Never));
    CHECK(minimizePersistenceValid(MinimizePersistence::Always));
    CHECK(!minimizePersistenceValid(MinimizePersistence::None));
}
