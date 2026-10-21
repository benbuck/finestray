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
#include "Settings.h"

// doctest
#include <doctest/doctest.h>

TEST_CASE("Settings::initDefaults is valid")
{
    Settings settings;
    settings.initDefaults();

    CHECK(settings.valid());
    CHECK(settings.version_ == 1);
    CHECK(settings.minimizePlacement_ == MinimizePlacement::TrayAndMenu);
    CHECK(settings.hotkeyMinimize_ == "alt ctrl shift down");
    CHECK(settings.hotkeyMinimizeAll_ == "alt ctrl shift right");
    CHECK(settings.hotkeyRestore_ == "alt ctrl shift up");
    CHECK(settings.hotkeyRestoreAll_ == "alt ctrl shift left");
    CHECK(settings.hotkeyMenu_ == "alt ctrl shift home");
    CHECK(settings.modifiersOverride_ == "alt ctrl shift");
    CHECK(settings.autoTrays_.empty());
}

TEST_CASE("Settings JSON round-trip")
{
    Settings settings;
    settings.initDefaults();

    Settings::AutoTray autoTray;
    autoTray.executable_ = "C:\\Windows\\notepad.exe";
    autoTray.windowClass_ = "Notepad";
    autoTray.windowTitle_ = ".* - Notepad$";
    autoTray.trayEvent_ = TrayEvent::OpenAndMinimize;
    autoTray.minimizePersistence_ = MinimizePersistence::Always;
    settings.addAutoTray(std::move(autoTray));

    const std::string json = settings.toJSON();
    CHECK(!json.empty());

    Settings restored;
    CHECK(restored.fromJSON(json));
    CHECK(restored == settings);
    CHECK(restored.valid());
}

TEST_CASE("Settings::fromJSON rejects malformed input")
{
    Settings settings;
    settings.initDefaults();
    CHECK(!settings.fromJSON("{bad json"));
}

TEST_CASE("Settings::normalize fixes bad placement")
{
    Settings settings;
    settings.initDefaults();
    settings.minimizePlacement_ = MinimizePlacement::None;
    settings.normalize();
    CHECK(settings.minimizePlacement_ == MinimizePlacement::TrayAndMenu);
}

TEST_CASE("Settings::normalize prunes empty auto-trays")
{
    Settings settings;
    settings.initDefaults();
    settings.addAutoTray(Settings::AutoTray {});
    settings.normalize();
    CHECK(settings.autoTrays_.empty());
}

TEST_CASE("Settings::normalize upgrades none event/persistence")
{
    Settings settings;
    settings.initDefaults();
    Settings::AutoTray autoTray;
    autoTray.executable_ = "C:\\Windows\\notepad.exe";
    autoTray.trayEvent_ = TrayEvent::None;
    autoTray.minimizePersistence_ = MinimizePersistence::None;
    settings.addAutoTray(std::move(autoTray));
    settings.normalize();
    REQUIRE(settings.autoTrays_.size() == 1);
    CHECK(settings.autoTrays_.at(0).trayEvent_ == TrayEvent::Minimize);
    CHECK(settings.autoTrays_.at(0).minimizePersistence_ == MinimizePersistence::Never);
}

TEST_CASE("Settings::valid rejects bad version")
{
    Settings settings;
    settings.initDefaults();
    settings.version_ = 999;
    CHECK(!settings.valid());
}

TEST_CASE("Settings::valid rejects bad hotkey")
{
    Settings settings;
    settings.initDefaults();
    settings.hotkeyMinimize_ = "foo bar";
    CHECK(!settings.valid());
}

TEST_CASE("Settings::valid rejects invalid title regex")
{
    Settings settings;
    settings.initDefaults();
    Settings::AutoTray autoTray;
    autoTray.executable_ = "C:\\Windows\\notepad.exe";
    autoTray.windowTitle_ = "(unclosed";
    settings.addAutoTray(std::move(autoTray));
    CHECK(!settings.valid());
}
