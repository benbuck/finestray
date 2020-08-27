// Copyright 2020 Benbuck Nason

#include "Hotkey.h"

#include <Windows.h>

Hotkey::Hotkey() : hwnd_(NULL), id_(-1) {}
Hotkey::~Hotkey() { destroy(); }

bool Hotkey::create(int id, HWND hwnd, INT hotkey, INT hotkeyModifiers)
{
    if (!RegisterHotKey(hwnd, id, hotkeyModifiers, hotkey)) {
        return false;
    }

    hwnd_ = hwnd;
    id_ = id;

    return true;
}

void Hotkey::destroy()
{
    if (id_ >= 0) {
        UnregisterHotKey(hwnd_, id_);

        id_ = -1;
        hwnd_ = NULL;
    }
}
