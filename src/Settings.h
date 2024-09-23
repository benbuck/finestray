// Copyright 2020 Benbuck Nason

#pragma once

// Windows
#include <Windows.h>

// Standard library
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
    bool writeToFile(const std::string & fileName);

    void dump();

    struct AutoTray
    {
        std::string executable_;
        std::string windowClass_;
        std::string windowTitle_;
    };

    void addAutoTray(const std::string & executable, const std::string & windowClass, const std::string & windowTitle);

    std::vector<AutoTray> autoTrays_;
    std::string hotkeyMinimize_;
    std::string hotkeyRestore_;
    std::string modifiersOverride_;
    unsigned int pollInterval_;

private:
    bool parseJson(const std::string & json);
    std::string constructJSON();
    static bool autoTrayItemCallback(const cJSON * cjson, void * userData);
};
