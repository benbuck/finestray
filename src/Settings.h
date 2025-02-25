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

#pragma once

// App
#include "MinimizePlacement.h"

// Standard library
#include <string>
#include <vector>

struct cJSON;

class Settings
{
public:
    Settings();
    ~Settings();

    bool operator==(const Settings & rhs) const;
    bool operator!=(const Settings & rhs) const;

    bool readFromFile(const std::string & fileName);
    bool writeToFile(const std::string & fileName);

    void normalize();
    void dump();

    struct AutoTray
    {
        bool operator==(const AutoTray & rhs) const;
        bool operator!=(const AutoTray & rhs) const;

        std::string executable_;
        std::string windowClass_;
        std::string windowTitle_;
    };

    void addAutoTray(const std::string & executable, const std::string & windowClass, const std::string & windowTitle);

    bool startWithWindows_;
    MinimizePlacement minimizePlacement_;
    std::string hotkeyMinimize_;
    std::string hotkeyRestore_;
    std::string modifiersOverride_;
    unsigned int pollInterval_; // zero to disable
    std::vector<AutoTray> autoTrays_;

private:
    bool parseJson(const std::string & json);
    std::string constructJSON();
    static bool autoTrayItemCallback(const cJSON * cjson, void * userData);
};
