// Copyright 2020 Benbuck Nason

#pragma once

#include <Windows.h>

#include <string>
#include <vector>

struct cJSON;

class Settings
{
public:
    Settings();
    ~Settings();

    bool readFromFile(const std::wstring & fileName);
    bool parseCommandLine(int argc, char const * const * argv);

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

    // "poll-millis"
    unsigned int pollMillis_;

    // "tray-icon"
    bool trayIcon_;

private:
    bool parseJson(const std::string & json);
    void addAutoTray(const std::string & className, const std::string & titleRegex);
    static bool autoTrayItemCallback(const cJSON * cjson, void * userData);
};
