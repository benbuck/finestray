// Copyright 2020 Benbuck Nason

#pragma once

// windows
#include <Windows.h>

// standard library
#include <string>

class Hotkey
{
public:
    Hotkey();
    ~Hotkey();

    bool create(int id, HWND hwnd, UINT hotkey, UINT hotkeyModifiers);
    void destroy();

    static bool parse(const std::string & hotkeyStr, UINT & key, UINT & modifiers);

private:
    HWND hwnd_;
    int id_;
};
