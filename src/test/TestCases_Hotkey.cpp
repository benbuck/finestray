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
#include "Hotkey.h"

// Windows
#include <Windows.h>

// doctest
#include <doctest/doctest.h>

TEST_CASE("Hotkey::valid")
{
    CHECK(Hotkey::valid("alt ctrl shift down"));
    CHECK(Hotkey::valid("alt ctrl shift f1"));
    CHECK(Hotkey::valid("none"));
    // empty disables the hotkey, so it is valid (see README modifiers/hotkeys)
    CHECK(Hotkey::valid(""));

    CHECK(!Hotkey::valid("none none"));
    CHECK(!Hotkey::valid("none alt"));
    CHECK(!Hotkey::valid("alt down up"));
    CHECK(!Hotkey::valid("bogus-hotkey-name"));
    CHECK(!Hotkey::valid("alt bogus-hotkey-name"));
}

TEST_CASE("Hotkey::normalize")
{
    CHECK(Hotkey::normalize("alt ctrl shift down") == "alt ctrl shift down");
    CHECK(Hotkey::normalize("ALT CTRL SHIFT DOWN") == "alt ctrl shift down");
    CHECK(Hotkey::normalize("none") == "none");
    // invalid input is returned unchanged
    CHECK(Hotkey::normalize("none none") == "none none");
}

TEST_CASE("Hotkey::parse function keys")
{
    UINT key = 0;
    UINT modifiers = 0;
    CHECK(Hotkey::parse("alt ctrl shift f1", key, modifiers));
    CHECK(key == VK_F1);
    CHECK(modifiers == (MOD_ALT | MOD_CONTROL | MOD_SHIFT));

    CHECK(Hotkey::parse("none", key, modifiers));
    CHECK(key == 0);
    CHECK(modifiers == 0);

    CHECK(!Hotkey::parse("bogus hotkey xyz", key, modifiers));
}

TEST_CASE("Hotkey::parse single character key")
{
    // Exact VK value is keyboard-layout dependent; assert shape only.
    UINT key = 0;
    UINT modifiers = 0;
    CHECK(Hotkey::parse("alt a", key, modifiers));
    CHECK(key == 'A');
    CHECK(modifiers == MOD_ALT);
}
