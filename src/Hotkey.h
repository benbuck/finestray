// Copyright 2020 Benbuck Nason

#pragma once

#include <Windows.h>

class Hotkey
{
public:
    Hotkey();
    ~Hotkey();

    bool create(int id_, HWND hwnd, INT hotkey, INT hotkeyModifiers);
    void destroy();

private:
    HWND hwnd_;
    int id_;
};
