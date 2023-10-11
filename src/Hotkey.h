// Copyright 2020 Benbuck Nason

#pragma once

#include <Windows.h>

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
