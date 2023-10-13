// Copyright 2020 Benbuck Nason

#pragma once

// windows
#include <Windows.h>

// standard library
#include <string>
#include <vector>

struct cJSON;

class Settings
{
public:
    Settings();
    ~Settings();

    bool readFromFile(const std::string & fileName);
    bool parseCommandLine(int argc, const char * const * argv);

    struct AutoTray
    {
        std::string executable_;
        std::string windowClass_;
        std::string windowTitle_;
    };
    std::vector<AutoTray> autoTrays_;
    std::string hotkeyMinimize_;
    std::string hotkeyRestore_;
    unsigned int pollInterval_;
    bool trayIcon_;

private:
    bool parseJson(const std::string & json);
    void addAutoTray(const std::string & executable, const std::string & windowClass, const std::string & windowTitle);
    static bool autoTrayItemCallback(const cJSON * cjson, void * userData);
};
