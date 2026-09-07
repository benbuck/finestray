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
#include "StringUtility.h"

// Windows
#include <Windows.h>

// doctest
#include <doctest/doctest.h>

TEST_CASE("StringUtility::toLower")
{
    CHECK(StringUtility::toLower("") == "");
    CHECK(StringUtility::toLower("abc") == "abc");
    CHECK(StringUtility::toLower("AbC") == "abc");
    CHECK(StringUtility::toLower("ALT CTRL SHIFT DOWN") == "alt ctrl shift down");
}

TEST_CASE("StringUtility::trim")
{
    CHECK(StringUtility::trim("") == "");
    CHECK(StringUtility::trim("abc") == "abc");
    CHECK(StringUtility::trim("  abc  ") == "abc");
    CHECK(StringUtility::trim("\t abc \t") == "abc");
    CHECK(StringUtility::trim("   ") == "");
}

TEST_CASE("StringUtility::split")
{
    CHECK(StringUtility::split("", " ") == std::vector<std::string> {});
    CHECK(StringUtility::split("   ", " ") == std::vector<std::string> {});
    CHECK(StringUtility::split("abc", " ") == std::vector<std::string> { "abc" });
    // consecutive delimiters do not produce empty tokens
    CHECK(StringUtility::split("a b  c", " ") == std::vector<std::string> { "a", "b", "c" });
    CHECK(StringUtility::split("a\tb c", " \t") == std::vector<std::string> { "a", "b", "c" });
}

TEST_CASE("StringUtility::join")
{
    CHECK(StringUtility::join({}, " ") == "");
    CHECK(StringUtility::join({ "a" }, " ") == "a");
    CHECK(StringUtility::join({ "a", "b", "c" }, " ") == "a b c");
    CHECK(StringUtility::join({ "alt", "ctrl" }, " ") == "alt ctrl");
}

TEST_CASE("StringUtility::boolToCString")
{
    CHECK(std::string(StringUtility::boolToCString(true)) == "true");
    CHECK(std::string(StringUtility::boolToCString(false)) == "false");
}

TEST_CASE("StringUtility wide/utf8 round-trip")
{
    const std::string utf8 = "hello";
    CHECK(StringUtility::wideStringToString(StringUtility::stringToWideString(utf8)) == utf8);
}

TEST_CASE("StringUtility::errorToString strips trailing newline")
{
    // Exact text is OS-locale dependent; assert shape instead.
    const std::string text = StringUtility::errorToString(2); // ERROR_FILE_NOT_FOUND
    CHECK(!text.empty());
    CHECK(text.back() != '\n');
    CHECK(text.back() != '\r');
}

TEST_CASE("StringUtility::lastErrorString format")
{
    SetLastError(2);
    const std::string text = StringUtility::lastErrorString();
    CHECK(text.rfind("2 - ", 0) == 0);
}
