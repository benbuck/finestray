// Copyright 2020 Benbuck Nason

#include "Hotkey.h"

#include "DebugPrint.h"
#include "StringUtilities.h"

#include <Windows.h>

#include <map>
#include <string>
#include <vector>

static const std::map<std::string, UINT> modifierMap_ = {
    { "alt", MOD_ALT },
    { "ctrl", MOD_CONTROL },
    { "shift", MOD_SHIFT },
    { "win", MOD_WIN },
};

static const std::map<std::string, UINT> vkeyMap_ = {
    { "back", VK_BACK },  { "esc", VK_ESCAPE },  { "f1", VK_F1 },     { "f2", VK_F2 },    { "f3", VK_F3 },
    { "f4", VK_F4 },      { "f5", VK_F5 },       { "f6", VK_F6 },     { "f7", VK_F7 },    { "f8", VK_F8 },
    { "f9", VK_F9 },      { "f10", VK_F10 },     { "f11", VK_F11 },   { "f12", VK_F12 },  { "f13", VK_F13 },
    { "f14", VK_F14 },    { "f15", VK_F15 },     { "f16", VK_F16 },   { "f17", VK_F17 },  { "f18", VK_F18 },
    { "f19", VK_F19 },    { "f20", VK_F20 },     { "f21", VK_F21 },   { "f22", VK_F22 },  { "f23", VK_F23 },
    { "f24", VK_F24 },    { "tab", VK_TAB },     { "left", VK_LEFT }, { "right", VK_UP }, { "up", VK_UP },
    { "down", VK_DOWN },  { "space", VK_SPACE }, { "home", VK_HOME }, { "end", VK_END },  { "ins", VK_INSERT },
    { "del", VK_DELETE },
};

Hotkey::Hotkey() : hwnd_(NULL), id_(-1) {}

Hotkey::~Hotkey() { destroy(); }

bool Hotkey::create(int id, HWND hwnd, UINT hotkey, UINT hotkeyModifiers)
{
    if (!RegisterHotKey(hwnd, id, hotkeyModifiers, hotkey)) {
        DEBUG_PRINTF("failed to register hotkey %d\n", id_);
        return false;
    }

    hwnd_ = hwnd;
    id_ = id;

    return true;
}

void Hotkey::destroy()
{
    if (id_ >= 0) {
        if (!UnregisterHotKey(hwnd_, id_)) {
            DEBUG_PRINTF("failed to unregister hotkey %d\n", id_);
        }

        id_ = -1;
        hwnd_ = NULL;
    }
}

bool Hotkey::parse(const std::string & hotkeyStr, UINT & key, UINT & modifiers)
{
    if (hotkeyStr.empty()) {
        return true;
    }

    key = 0;
    modifiers = 0;

    if (StringUtilities::toLower(hotkeyStr) == "none") {
        return true;
    }

    std::vector<std::string> tokens = StringUtilities::split(hotkeyStr, "+");
    for (auto token : tokens) {
        token = StringUtilities::trim(token);
        token = StringUtilities::toLower(token);

        // look for modifier string
        const auto & mit = modifierMap_.find(token);
        if (mit != modifierMap_.end()) {
            modifiers |= mit->second;
        } else {
            // look for vkey string
            const auto & vkit = vkeyMap_.find(token);
            if (vkit != vkeyMap_.end()) {
                if (key) {
                    DEBUG_PRINTF("more than one key in hotkey\n");
                    return false;
                }
                key = vkit->second;
            } else {
                // look for key character
                if (token.length() == 1) {
                    if (key) {
                        DEBUG_PRINTF("more than one key in hotkey\n");
                        return false;
                    }
                    key = VkKeyScanA(token[0]);
                    if (key == 0xFFFF) {
                        DEBUG_PRINTF("unknown key %s in hotkey\n", token.c_str());
                        return false;
                    }
                } else {
                    DEBUG_PRINTF("unknown value in hotkey\n");
                    return false;
                }
            }
        }
    }

    if (!key || !modifiers) {
        return false;
    }

    return true;
}
