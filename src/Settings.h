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
#include "TrayEvent.h"

// Standard library
#include <string>
#include <vector>

struct cJSON;

class Settings
{
public:
    Settings() = default;

    bool operator==(const Settings & rhs) const = default;
    bool operator!=(const Settings & rhs) const = default;

    void initDefaults();

    bool fromJSON(const std::string & json);
    std::string toJSON() const;

    bool valid() const;

    void normalize();
    void dump() const noexcept;

    struct AutoTray
    {
        bool operator==(const AutoTray & rhs) const = default;
        bool operator!=(const AutoTray & rhs) const = default;

        std::string executable_;
        std::string windowClass_;
        std::string windowTitle_;

        TrayEvent trayEvent_ { TrayEvent::Minimize };
    };

    void addAutoTray(AutoTray && autoTray);

    static bool fileExists(const std::string & fileName);

    unsigned int version_ {};
    bool startWithWindows_ {};
    bool showWindowsInMenu_ {};
    bool logToFile_ {};
    MinimizePlacement minimizePlacement_ {};
    std::string hotkeyMinimize_;
    std::string hotkeyMinimizeAll_;
    std::string hotkeyRestore_;
    std::string hotkeyRestoreAll_;
    std::string hotkeyMenu_;
    std::string modifiersOverride_;
    std::vector<AutoTray> autoTrays_;
};
