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

#include "Settings.h"

// App
#include "AppName.h"
#include "File.h"
#include "Hotkey.h"
#include "Log.h"
#include "Path.h"

// cJSON
#include <cJSON.h>

// Windows
#include <Windows.h>
#include <shellapi.h>

// Standard library
#include <cstdlib>

namespace
{

bool getBool(const cJSON * cjson, const char * key, bool defaultValue);
double getNumber(const cJSON * cjson, const char * key, double defaultValue);
const char * getString(const cJSON * cjson, const char * key, const char * defaultValue);
void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void *);

enum SettingKeys : unsigned int
{
    SK_StartWithWindows,
    SK_LogToFile,
    SK_MinimizePlacement,
    SK_Executable,
    SK_WindowClass,
    SK_WindowTitle,
    SK_HotkeyMinimize,
    SK_HotkeyRestore,
    SK_ModifiersOverride,
    SK_PollInterval,
    SK_AutoTray,

    SK_Count
};

const bool startWithWindowsDefault_ = false;
const bool logToFileDefault_ = false;
const MinimizePlacement minimizePlacementDefault_ = MinimizePlacement::TrayAndMenu;
const char hotkeyMinimizeDefault_[] = "alt ctrl shift down";
const char hotkeyRestoreDefault_[] = "alt ctrl shift up";
const char modifiersOverrideDefault_[] = "alt ctrl shift";
const unsigned int pollIntervalDefault_ = 500;
const bool settingsIsFlag_[SK_Count] = { true, false, false, false, false, false, false, false, false, false };
const char * settingKeys_[SK_Count] = { "start-with-windows", "log-to-file",   "minimize-placement", "executable",
                                        "window-class",       "window-title",  "hotkey-minimize",    "hotkey-restore",
                                        "modifiers-override", "poll-interval", "auto-tray" };

} // anonymous namespace

Settings::Settings()
    : startWithWindows_(startWithWindowsDefault_)
    , logToFile_(logToFileDefault_)
    , minimizePlacement_(minimizePlacementDefault_)
    , hotkeyMinimize_(hotkeyMinimizeDefault_)
    , hotkeyRestore_(hotkeyRestoreDefault_)
    , modifiersOverride_(modifiersOverrideDefault_)
    , pollInterval_(pollIntervalDefault_)
    , autoTrays_()
{
}

Settings::~Settings()
{
}

bool Settings::operator==(const Settings & rhs) const
{
    if (this == &rhs) {
        return true;
    }

    return (startWithWindows_ == rhs.startWithWindows_) && (logToFile_ == rhs.logToFile_) &&
        (minimizePlacement_ == rhs.minimizePlacement_) && (hotkeyMinimize_ == rhs.hotkeyMinimize_) &&
        (hotkeyRestore_ == rhs.hotkeyRestore_) && (modifiersOverride_ == rhs.modifiersOverride_) &&
        (pollInterval_ == rhs.pollInterval_) && (autoTrays_ == rhs.autoTrays_);
}

bool Settings::readFromFile(const std::string & fileName)
{
    DEBUG_PRINTF("Reading settings from file: %s\n", fileName.c_str());

    std::string json;
    json = fileRead(fileName);
    if (json.empty()) {
        std::string appDataPath = pathJoin(getAppDataPath(), APP_NAME);
        std::string appDataFileName = pathJoin(appDataPath, fileName);
        json = fileRead(appDataFileName);
        if (json.empty()) {
            return false;
        }
    }

    return parseJson(json);
}

bool Settings::writeToFile(const std::string & fileName)
{
    DEBUG_PRINTF("Writing settings to file %s\n", fileName.c_str());

    normalize();

    std::string json = constructJSON();
    if (json.empty()) {
        return false;
    }

    if (!fileWrite(fileName, json)) {
        std::string appDataPath = pathJoin(getAppDataPath(), APP_NAME);
        if (!directoryExists(appDataPath)) {
            if (!CreateDirectoryA(appDataPath.c_str(), nullptr)) {
                WARNING_PRINTF(
                    "could not create directory '%s', CreateDirectoryA() failed: %s\n",
                    appDataPath.c_str(),
                    GetLastError());
                return false;
            }
        }
        std::string appDataFileName = pathJoin(appDataPath, fileName);
        if (!fileWrite(appDataFileName, json)) {
            return false;
        }
    }

    return true;
}

void Settings::normalize()
{
    switch (minimizePlacement_) {
        case MinimizePlacement::Tray:
        case MinimizePlacement::Menu:
        case MinimizePlacement::TrayAndMenu: {
            // no change
            break;
        }

        case MinimizePlacement::None:
        default: {
            DEBUG_PRINTF("Fixing bad minimize placement: %d\n", minimizePlacement_);
            minimizePlacement_ = minimizePlacementDefault_;
            break;
        }
    }

    hotkeyMinimize_ = Hotkey::normalize(hotkeyMinimize_);
    hotkeyRestore_ = Hotkey::normalize(hotkeyRestore_);
    modifiersOverride_ = Hotkey::normalize(modifiersOverride_);

    for (auto it = autoTrays_.begin(); it != autoTrays_.end();) {
        AutoTray & autoTray = *it;
        if (!autoTray.executable_.empty() || !autoTray.windowClass_.empty() || !autoTray.windowTitle_.empty()) {
            ++it;
        } else {
            DEBUG_PRINTF("Removing empty auto-tray item\n");
            it = autoTrays_.erase(it);
        }
    }
}

void Settings::dump()
{
#if !defined(NDEBUG)
    DEBUG_PRINTF("Settings:\n");
    DEBUG_PRINTF("\t%s: %s\n", settingKeys_[SK_StartWithWindows], startWithWindows_ ? "true" : "false");
    DEBUG_PRINTF("\t%s: %s\n", settingKeys_[SK_LogToFile], logToFile_ ? "true" : "false");
    DEBUG_PRINTF("\t%s: %s\n", settingKeys_[SK_MinimizePlacement], minimizePlacementToString(minimizePlacement_).c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str());
    DEBUG_PRINTF("\t%s: '%s'\n", settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str());
    DEBUG_PRINTF("\t%s: %u\n", settingKeys_[SK_PollInterval], pollInterval_);

    for (const AutoTray & autoTray : autoTrays_) {
        DEBUG_PRINTF("\t%s:\n", settingKeys_[SK_AutoTray]);
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_Executable], autoTray.executable_.c_str());
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_WindowClass], autoTray.windowClass_.c_str());
        DEBUG_PRINTF("\t\t%s: '%s'\n", settingKeys_[SK_WindowTitle], autoTray.windowTitle_.c_str());
    }
#endif
}

void Settings::addAutoTray(const std::string & executable, const std::string & windowClass, const std::string & windowTitle)
{
    AutoTray autoTray;
    autoTray.executable_ = executable;
    autoTray.windowClass_ = windowClass;
    autoTray.windowTitle_ = windowTitle;
    autoTrays_.push_back(autoTray);
}

bool Settings::parseJson(const std::string & json)
{
    const cJSON * cjson = cJSON_Parse(json.c_str());
    if (!cjson) {
        WARNING_PRINTF("failed to parse settings JSON:\n%s\n", cJSON_GetErrorPtr());
        return false;
    }

    DEBUG_PRINTF("parsed settings JSON:\n%s\n", cJSON_Print(cjson));

    startWithWindows_ = getBool(cjson, settingKeys_[SK_StartWithWindows], startWithWindows_);
    logToFile_ = getBool(cjson, settingKeys_[SK_LogToFile], logToFile_);

    const std::string & minimizePlacementString =
        getString(cjson, settingKeys_[SK_MinimizePlacement], minimizePlacementToString(minimizePlacement_).c_str());
    minimizePlacement_ = minimizePlacementFromString(minimizePlacementString);
    if (minimizePlacement_ == MinimizePlacement::None) {
        WARNING_PRINTF("bad %s argument: %s\n", settingKeys_[SK_MinimizePlacement], minimizePlacementString.c_str());
    }

    hotkeyMinimize_ = getString(cjson, settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str());
    hotkeyRestore_ = getString(cjson, settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str());
    modifiersOverride_ = getString(cjson, settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str());
    pollInterval_ = (unsigned int)getNumber(cjson, settingKeys_[SK_PollInterval], (double)pollInterval_);

    const cJSON * autotray = cJSON_GetObjectItemCaseSensitive(cjson, settingKeys_[SK_AutoTray]);
    if (autotray) {
        iterateArray(autotray, autoTrayItemCallback, this);
    }

    normalize();

    return true;
}

bool Settings::fileExists(const std::string & fileName)
{
    std::string path = getWriteablePath();
    if (path.empty()) {
        return false;
    }

    std::string fullPath = pathJoin(path, fileName);
    return ::fileExists(fullPath);
}

std::string Settings::constructJSON()
{
    cJSON * cjson = cJSON_CreateObject();
    if (!cjson) {
        WARNING_PRINTF("failed to create settings JSON object\n");
        return std::string();
    }

    bool fail = false;

    if (!cJSON_AddBoolToObject(cjson, settingKeys_[SK_StartWithWindows], startWithWindows_)) {
        fail = true;
    }

    if (!cJSON_AddBoolToObject(cjson, settingKeys_[SK_LogToFile], logToFile_)) {
        fail = true;
    }

    if (!cJSON_AddStringToObject(
            cjson,
            settingKeys_[SK_MinimizePlacement],
            minimizePlacementToString(minimizePlacement_).c_str())) {
        fail = true;
    }

    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyMinimize], hotkeyMinimize_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_HotkeyRestore], hotkeyRestore_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddStringToObject(cjson, settingKeys_[SK_ModifiersOverride], modifiersOverride_.c_str())) {
        fail = true;
    }
    if (!cJSON_AddNumberToObject(cjson, settingKeys_[SK_PollInterval], (double)pollInterval_)) {
        fail = true;
    }

    if (!autoTrays_.empty()) {
        cJSON * autotrayArray = cJSON_AddArrayToObject(cjson, settingKeys_[SK_AutoTray]);
        if (!autotrayArray) {
            fail = true;
        } else {
            for (const AutoTray & autoTray : autoTrays_) {
                cJSON * item = cJSON_CreateObject();
                if (!item) {
                    fail = true;
                    break;
                }

                if (!autoTray.executable_.empty() &&
                    !cJSON_AddStringToObject(item, settingKeys_[SK_Executable], autoTray.executable_.c_str())) {
                    fail = true;
                }

                if (!autoTray.windowClass_.empty() &&
                    !cJSON_AddStringToObject(item, settingKeys_[SK_WindowClass], autoTray.windowClass_.c_str())) {
                    fail = true;
                }

                if (!autoTray.windowTitle_.empty() &&
                    !cJSON_AddStringToObject(item, settingKeys_[SK_WindowTitle], autoTray.windowTitle_.c_str())) {
                    fail = true;
                }

                if (!cJSON_AddItemToArray(autotrayArray, item)) {
                    fail = true;
                }
            }
        }
    }

    if (fail) {
        WARNING_PRINTF("failed to construct json settings\n");
        cJSON_Delete(cjson);
        return std::string();
    }

    return cJSON_Print(cjson);
}

bool Settings::autoTrayItemCallback(const cJSON * cjson, void * userData)
{
    if (!cJSON_IsObject(cjson)) {
        WARNING_PRINTF("bad type for '%s'\n", cjson->string);
        return false;
    }

    const char * executable = getString(cjson, settingKeys_[SK_Executable], nullptr);
    const char * windowClass = getString(cjson, settingKeys_[SK_WindowClass], nullptr);
    const char * windowTitle = getString(cjson, settingKeys_[SK_WindowTitle], nullptr);
    if (executable || windowClass || windowTitle) {
        Settings * settings = (Settings *)userData;
        settings->addAutoTray(executable ? executable : "", windowClass ? windowClass : "", windowTitle ? windowTitle : "");
    }

    return true;
}

bool Settings::AutoTray::operator==(const AutoTray & rhs) const
{
    if (this == &rhs) {
        return true;
    }
    return (executable_ == rhs.executable_) && (windowClass_ == rhs.windowClass_) && (windowTitle_ == rhs.windowTitle_);
}

bool Settings::AutoTray::operator!=(const AutoTray & rhs) const
{
    return !(*this == rhs);
}

namespace
{

bool getBool(const cJSON * cjson, const char * key, bool defaultValue)
{
    const cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    if (!cJSON_IsBool(item)) {
        WARNING_PRINTF("bad type for '%s'\n", item->string);
        return defaultValue;
    }

    return cJSON_IsTrue(item) ? true : false;
}

double getNumber(const cJSON * cjson, const char * key, double defaultValue)
{
    const cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    if (!cJSON_IsNumber(item)) {
        WARNING_PRINTF("bad type for '%s'\n", item->string);
        return defaultValue;
    }

    return cJSON_GetNumberValue(item);
}

const char * getString(const cJSON * cjson, const char * key, const char * defaultValue)
{
    cJSON * item = cJSON_GetObjectItemCaseSensitive(cjson, key);
    if (!item) {
        return defaultValue;
    }

    const char * str = cJSON_GetStringValue(item);
    if (!str) {
        WARNING_PRINTF("bad type for '%s'\n", cjson->string);
        return defaultValue;
    }

    return str;
}

void iterateArray(const cJSON * cjson, bool (*callback)(const cJSON *, void *), void * userData)
{
    if (!cJSON_IsArray(cjson)) {
        WARNING_PRINTF("bad type for '%s'\n", cjson->string);
    } else {
        int arrSize = cJSON_GetArraySize(cjson);
        for (int i = 0; i < arrSize; ++i) {
            const cJSON * item = cJSON_GetArrayItem(cjson, i);
            if (!callback(item, userData)) {
                break;
            }
        }
    }
}

} // anonymous namespace
