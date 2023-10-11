// Copyright 2020 Benbuck Nason

#pragma once

#include <Windows.h>

#include <string>
#include <vector>

struct Settings
{
    Settings();
    ~Settings();

    void parseCommandLine();
    void parseJson(const char * json);
    void addAutoTray(const std::string & className, const std::string & titleRegex);

    // "exit", command line only
    bool shouldExit_;

    // "auto-tray"
    struct AutoTray
    {
        // "class-name"
        std::string className_;

        // "title-regex"
        std::string titleRegex_;
    };
    std::vector<AutoTray> autoTrays_;

    // "hotkey-minimize"
    std::string hotkeyMinimize_;

    // "hotkey-restore"
    std::string hotkeyRestore_;

    // "enum-windows-interval-ms"
    unsigned int enumWindowsIntervalMs_;

    // "password"
    std::string password_;

    // "tray-icon"
    bool trayIcon_;
};
