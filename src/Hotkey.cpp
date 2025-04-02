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
#include "Log.h"
#include "StringUtility.h"

// Windows
#include <Windows.h>

// Standard library
#include <map>

namespace
{

const std::map<std::string, UINT> modifierMap_ = { { "alt", MOD_ALT },
                                                   { "ctrl", MOD_CONTROL },
                                                   { "shift", MOD_SHIFT },
                                                   { "win", MOD_WIN } };

const std::map<std::string, UINT> vkeyMap_ = {
    { "back", VK_BACK }, { "esc", VK_ESCAPE },  { "f1", VK_F1 },     { "f2", VK_F2 },       { "f3", VK_F3 },
    { "f4", VK_F4 },     { "f5", VK_F5 },       { "f6", VK_F6 },     { "f7", VK_F7 },       { "f8", VK_F8 },
    { "f9", VK_F9 },     { "f10", VK_F10 },     { "f11", VK_F11 },   { "f12", VK_F12 },     { "f13", VK_F13 },
    { "f14", VK_F14 },   { "f15", VK_F15 },     { "f16", VK_F16 },   { "f17", VK_F17 },     { "f18", VK_F18 },
    { "f19", VK_F19 },   { "f20", VK_F20 },     { "f21", VK_F21 },   { "f22", VK_F22 },     { "f23", VK_F23 },
    { "f24", VK_F24 },   { "tab", VK_TAB },     { "left", VK_LEFT }, { "right", VK_RIGHT }, { "up", VK_UP },
    { "down", VK_DOWN }, { "space", VK_SPACE }, { "home", VK_HOME }, { "end", VK_END },     { "ins", VK_INSERT },
    { "del", VK_DELETE }
};

} // anonymous namespace

bool Hotkey::create(int id, HWND hwnd, UINT hotkey, UINT hotkeyModifiers)
{
    DEBUG_PRINTF("creating hotkey %d\n", id);

    hwnd_ = hwnd;
    id_ = id;

    if (!RegisterHotKey(hwnd, id, hotkeyModifiers, hotkey)) {
        WARNING_PRINTF(
            "failed to register hotkey %d, RegisterHotKey() failed: %s\n",
            id_,
            StringUtility::lastErrorString().c_str());
        return false;
    }

    return true;
}

void Hotkey::destroy() noexcept
{
    if (id_ >= 0) {
        DEBUG_PRINTF("destroying hotkey %d\n", id_);
        if (!UnregisterHotKey(hwnd_, id_)) {
            WARNING_PRINTF("failed to unregister hotkey %d, UnregisterHotKey failed: %lu\n", id_, GetLastError());
        }

        id_ = -1;
        hwnd_ = nullptr;
    }
}

bool Hotkey::valid(const std::string & hotkeyStr)
{
    const ParseResult parseResult = parseInternal(hotkeyStr);
    return parseResultValid(parseResult);
}

std::string Hotkey::normalize(const std::string & hotkeyStr)
{
    std::string normalized;

    ParseResult parseResult = parseInternal(hotkeyStr);
    if (!parseResultValid(parseResult)) {
        return hotkeyStr;
    }

    if (parseResult.noneCount) {
        normalized = "none";
    } else {
        normalized = StringUtility::join(parseResult.modifiers, " ");

        if (!parseResult.keys.empty()) {
            if (!normalized.empty()) {
                normalized += ' ';
            }
            normalized += parseResult.keys.at(0);
        }
    }

    if (hotkeyStr == normalized) {
        DEBUG_PRINTF("hotkey '%s' already normalized\n", hotkeyStr.c_str());
    } else {
        DEBUG_PRINTF("normalized hotkey '%s' to '%s'\n", hotkeyStr.c_str(), normalized.c_str());
    }
    return normalized;
}

bool Hotkey::parse(const std::string & hotkeyStr, UINT & key, UINT & modifiers)
{
    key = 0;
    modifiers = 0;

    ParseResult parseResult = parseInternal(hotkeyStr);
    if (!parseResultValid(parseResult)) {
        return false;
    }

    for (const std::string & modifier : parseResult.modifiers) {
        const std::map<std::string, UINT>::const_iterator & mit = modifierMap_.find(modifier);
        modifiers |= mit->second;
    }

    if (!parseResult.keys.empty()) {
        std::string vkey = parseResult.keys.at(0);
        const std::map<std::string, UINT>::const_iterator & vkit = vkeyMap_.find(vkey);
        if (vkit != vkeyMap_.end()) {
            key = vkit->second;
        } else {
            key = VkKeyScanA(vkey.at(0));
        }
    }

    DEBUG_PRINTF("parsed hotkey '%s' to key %#x and modifiers %#x\n", hotkeyStr.c_str(), key, modifiers);
    return true;
}

Hotkey::ParseResult Hotkey::parseInternal(const std::string & hotkeyStr)
{
    ParseResult parseResult;

    const std::string tmp = StringUtility::toLower(hotkeyStr);

    const std::vector<std::string> tokens = StringUtility::split(tmp, " \t");
    for (std::string token : tokens) {
        token = StringUtility::trim(token);
        token = StringUtility::toLower(token);

        if (token == "none") {
            ++parseResult.noneCount;
            continue;
        }

        // look for modifier string
        const std::map<std::string, UINT>::const_iterator & mit = modifierMap_.find(token);
        if (mit != modifierMap_.end()) {
            parseResult.modifiers.push_back(token);
            continue;
        }

        // look for vkey string
        const std::map<std::string, UINT>::const_iterator & vkit = vkeyMap_.find(token);
        if (vkit != vkeyMap_.end()) {
            parseResult.keys.push_back(token);
            continue;
        }

        // look for key character
        if (token.length() != 1) {
            parseResult.unrecognized.push_back(token);
            continue;
        }

        SHORT const scan = VkKeyScanA(token.at(0));
        if (static_cast<unsigned int>(scan) == 0xFFFF) {
            parseResult.unrecognized.push_back(token);
            continue;
        }

        parseResult.keys.push_back(token);
    }

    return parseResult;
}

bool Hotkey::parseResultValid(const ParseResult & parseResult)
{
    if (parseResult.noneCount > 1) {
        WARNING_PRINTF("hotkey has multiple 'none', not valid\n");
        return false;
    }

    if (parseResult.noneCount && (!parseResult.keys.empty() || !parseResult.modifiers.empty())) {
        WARNING_PRINTF(
            "hotkey has 'none' with other keys ('%s') and/or modifiers ('%s'), not valid\n",
            StringUtility::join(parseResult.keys, " ").c_str(),
            StringUtility::join(parseResult.modifiers, " ").c_str());
        return false;
    }

    if (parseResult.keys.size() > 1) {
        WARNING_PRINTF("hotkey has multiple keys ('%s'), not valid\n", StringUtility::join(parseResult.keys, " ").c_str());
        return false;
    }

    if (!parseResult.unrecognized.empty()) {
        WARNING_PRINTF(
            "hotkey has unrecognized strings ('%s'), not valid\n",
            StringUtility::join(parseResult.unrecognized, " ").c_str());
        return false;
    }

    return true;
}
